#include "Glob.h"
#include "Profiler.h"

#include <algorithm>

USE_MODULE(Arcb::Glob);

/**
 * @file Glob.cpp
 * @brief Glob parsing, expansion and glob-to-glob mapping utilities.
 *
 * This translation unit implements:
 * - Parsing of glob patterns into an internal representation (Pattern/Segment/Atom).
 * - Expansion of a parsed pattern against the filesystem.
 * - Mapping a list of paths matching one glob to another glob by capturing and instantiating wildcards.
 *
 * Notes about semantics:
 * - Segments are separated by the configured separator (typically '/').
 * - '*' and '?' are segment-local wildcards (they do not cross separators).
 * - "**" is treated as a special "double star" segment that can match 0..N path segments.
 * - Character classes like "[a-z]" are supported with optional backslash escaping.
 * - Expansion output is deterministic (sorted + deduplicated).
 */



//    ██████╗ ██████╗ ██╗██╗   ██╗ █████╗ ████████╗███████╗    ███████╗██╗   ██╗███╗   ██╗ ██████╗███████╗
//    ██╔══██╗██╔══██╗██║██║   ██║██╔══██╗╚══██╔══╝██╔════╝    ██╔════╝██║   ██║████╗  ██║██╔════╝██╔════╝
//    ██████╔╝██████╔╝██║██║   ██║███████║   ██║   █████╗      █████╗  ██║   ██║██╔██╗ ██║██║     ███████╗
//    ██╔═══╝ ██╔══██╗██║╚██╗ ██╔╝██╔══██║   ██║   ██╔══╝      ██╔══╝  ██║   ██║██║╚██╗██║██║     ╚════██║
//    ██║     ██║  ██║██║ ╚████╔╝ ██║  ██║   ██║   ███████╗    ██║     ╚██████╔╝██║ ╚████║╚██████╗███████║
//    ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═══╝  ╚═╝  ╚═╝   ╚═╝   ╚══════╝    ╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝╚══════╝




//    ██████╗  █████╗ ██████╗ ███████╗███████╗██████╗
//    ██╔══██╗██╔══██╗██╔══██╗██╔════╝██╔════╝██╔══██╗
//    ██████╔╝███████║██████╔╝███████╗█████╗  ██████╔╝
//    ██╔═══╝ ██╔══██║██╔══██╗╚════██║██╔══╝  ██╔══██╗
//    ██║     ██║  ██║██║  ██║███████║███████╗██║  ██║
//    ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝╚═╝  ╚═╝




/**
 * @brief Check whether a character is a glob metacharacter.
 *
 * This helper defines the set of characters that can be escaped when
 * backslash escaping is enabled.
 *
 * @param c Input character.
 * @return true if the character is part of the glob syntax.
 */
static bool IsMeta(char c) noexcept
{
    // CHECK AGAINST THE GLOB META SET
    switch (c)
    {
        case '*':
        case '?':
        case '[':
        case ']':
        case '\\':
            return true;

        default:
            return false;
    }
}



/**
 * @brief Normalize an input pattern according to parsing options.
 *
 * Currently this performs a minimal normalization:
 * - If separator is '/', convert backslashes to '/' to align with internal generic paths.
 *
 * @param input Raw user pattern.
 * @param opt Parsing options.
 * @return A normalized pattern string.
 */
static std::string Normalize(std::string_view input, const Options& opt) noexcept
{
    std::string s;
    s.reserve(input.size());

    // REWRITE PATH SEPARATORS IF REQUESTED
    for (char c : input)
    {
        if (c == '\\' && opt.separator == '/')
        {
            s.push_back('/');
        }
        else
        {
            s.push_back(c);
        }
    }

    return s;
}



/**
 * @brief Emit a pending literal accumulator into a segment, if any.
 *
 * The parser accumulates runs of non-meta characters into a single literal atom,
 * flushing it before emitting wildcard atoms.
 *
 * @param lit Mutable literal accumulator.
 * @param out Target segment being built.
 * @return true always, kept as a boolean hook for early-return patterns.
 */
static bool EmitLiteralIfAny(std::string& lit, Segment& out) noexcept
{
    // FAST-PATH: NOTHING TO FLUSH
    if (lit.empty())
    {
        return true;
    }

    // EMIT A SINGLE LITERAL ATOM AND CLEAR THE ACCUMULATOR
    out.atoms.push_back(Atom::MakeLiteral(std::move(lit)));
    lit.clear();
    return true;
}



/**
 * @brief Parse a character class starting at '[' within a segment.
 *
 * Supported syntax:
 * - Optional leading '^' to negate the class.
 * - Singles and ranges (e.g. a-z).
 * - Optional backslash escapes for meta characters if enabled.
 *
 * On success the function leaves @p i positioned on the closing ']' character.
 *
 * @param seg Segment view being parsed.
 * @param i   Current index in @p seg (must point to '[' on entry).
 * @param out Output character class structure.
 * @param base_offset Offset of this segment in the full normalized pattern (for error reporting).
 * @param err Output parse error (code + absolute offset).
 * @param opt Parsing options.
 * @return true on success, false on parse error.
 */
static bool ParseCharClass(std::string_view seg, std::size_t& i, CharClass& out, std::size_t base_offset, ParseError& err, const Options& opt) noexcept
{
    // RESET OUTPUT
    out = CharClass{};

    // SAVE START OFFSET FOR DIAGNOSTICS
    std::size_t start = i;

    // CONSUME '['
    i += 1;
    if (i >= seg.size())
    {
        err.code   = ParseError::Code::UNCLOSED_CHARCLASS;
        err.offset = base_offset + start;
        return false;
    }

    // OPTIONAL NEGATION MARKER
    if (seg[i] == '^')
    {
        out.negated = true;
        i += 1;
    }

    // MUST HAVE AT LEAST ONE TOKEN BEFORE ']'
    if (i >= seg.size())
    {
        err.code   = ParseError::Code::UNCLOSED_CHARCLASS;
        err.offset = base_offset + start;
        return false;
    }

    bool any = false;

    // READ A SINGLE LOGICAL CHARACTER (HANDLES OPTIONAL BACKSLASH ESCAPE)
    auto read_char = [&] (char& ch) noexcept -> bool
    {
        if (i >= seg.size())
        {
            return false;
        }

        char c = seg[i];

        if (opt.backslash_escape && c == '\\')
        {
            if (i + 1 >= seg.size())
            {
                return false;
            }
            char n = seg[i + 1];
            ch = n;
            i += 2;
            return true;
        }

        ch = c;
        i += 1;
        return true;
    };

    // PARSE UNTIL CLOSING ']'
    while (i < seg.size())
    {
        // TERMINATE ON ']'
        if (seg[i] == ']')
        {
            // REJECT EMPTY CLASSES LIKE "[]"
            if (!any)
            {
                err.code   = ParseError::Code::EMPTY_CHARCLASS;
                err.offset = base_offset + start;
                return false;
            }

            // LEAVE i ON THE ']'
            return true;
        }

        // READ FIRST CHAR OR RANGE START
        char first = '\0';
        if (!read_char(first))
        {
            err.code   = ParseError::Code::UNCLOSED_CHARCLASS;
            err.offset = base_offset + start;
            return false;
        }

        // RANGE DETECTION: first '-' last (WITH GUARDS AGAINST "a-]")
        if (i < seg.size() && seg[i] == '-' && (i + 1) < seg.size() && seg[i + 1] != ']')
        {
            // CONSUME '-'
            i += 1;

            char last = '\0';
            if (!read_char(last))
            {
                err.code   = ParseError::Code::INVALID_RANGE;
                err.offset = base_offset + i;
                return false;
            }

            // VALIDATE ORDERING
            if (static_cast<unsigned char>(first) > static_cast<unsigned char>(last))
            {
                err.code   = ParseError::Code::INVALID_RANGE;
                err.offset = base_offset + (i - 1);
                return false;
            }

            // EMIT RANGE
            out.ranges.push_back(CharRange{first, last});
            any = true;
            continue;
        }

        // EMIT SINGLE CHARACTER
        out.singles.push_back(first);
        any = true;
    }

    // IF WE EXIT THE LOOP, ']' WAS NOT FOUND
    err.code   = ParseError::Code::UNCLOSED_CHARCLASS;
    err.offset = base_offset + start;

    return false;
}



/**
 * @brief Parse a single path segment into a sequence of atoms.
 *
 * The segment is a substring between separators. This parser emits:
 * - Literal atoms (runs of non-meta characters)
 * - STAR ('*'), QMARK ('?'), CHARCLASS ('[...]')
 * - Optional DOUBLESTAR when segment-only mode is enabled and the segment is exactly "**"
 *
 * @param seg Segment substring (no separators).
 * @param out Output segment.
 * @param base_offset Offset in the full normalized pattern for diagnostics.
 * @param err Output parse error.
 * @param opt Parsing options.
 * @return true on success, false on parse error.
 */
static bool ParseSegment(std::string_view seg, Segment& out, std::size_t base_offset, ParseError& err, const Options& opt) noexcept
{
    // RESET OUTPUT
    out = Segment{};

    // EMPTY SEGMENTS ARE ACCEPTED (THE CALLER CONTROLS SPLITTING RULES)
    if (seg.empty())
    {
        return true;
    }

    // SPECIAL CASE: DOUBLESTAR AS A WHOLE SEGMENT
    if (opt.doublestar_segment_only && seg == "**")
    {
        out.atoms.push_back(Atom::MakeDoubleStar());
        return true;
    }

    std::string literal;
    literal.reserve(seg.size());

    // WALK THE SEGMENT LEFT-TO-RIGHT EMITTING ATOMS
    for (std::size_t i = 0; i < seg.size(); ++i)
    {
        char c = seg[i];

        // HANDLE BACKSLASH-ESCAPED META CHARS
        if (opt.backslash_escape && c == '\\')
        {
            if (i + 1 >= seg.size())
            {
                err.code   = ParseError::Code::INVALID_ESCAPE;
                err.offset = base_offset + i;
                return false;
            }

            char n = seg[i + 1];

            if (!IsMeta(n))
            {
                err.code   = ParseError::Code::INVALID_ESCAPE;
                err.offset = base_offset + i;
                return false;
            }

            // APPEND ESCAPED META AS LITERAL
            literal.push_back(n);
            i += 1;
            continue;
        }

        // STAR ATOM
        if (c == '*')
        {
            if (!EmitLiteralIfAny(literal, out))
            {
                return false;
            }

            out.atoms.push_back(Atom::MakeStar());
            continue;
        }

        // QMARK ATOM
        if (c == '?')
        {
            if (!EmitLiteralIfAny(literal, out))
            {
                return false;
            }

            out.atoms.push_back(Atom::MakeQMark());
            continue;
        }

        // CHARCLASS ATOM
        if (c == '[')
        {
            if (!EmitLiteralIfAny(literal, out))
            {
                return false;
            }

            CharClass cls{};
            if (!ParseCharClass(seg, i, cls, base_offset, err, opt))
            {
                return false;
            }

            out.atoms.push_back(Atom::MakeCharClass(std::move(cls)));
            continue;
        }

        // LITERAL CHARACTER
        literal.push_back(c);
    }

    // FLUSH LAST LITERAL RUN
    if (!EmitLiteralIfAny(literal, out))
    {
        return false;
    }

    // VALIDATE DOUBLESTAR USAGE WHEN IT IS RESTRICTED TO WHOLE SEGMENTS
    if (opt.doublestar_segment_only)
    {
        for (const auto& a : out.atoms)
        {
            if (a.kind == Atom::Kind::STAR)
            {
                // STAR IS ALWAYS ALLOWED
            }
            if (a.kind == Atom::Kind::LITERAL && a.literal.find("**") != std::string::npos)
            {
                err.code   = ParseError::Code::INVALID_DOUBLESTAR;
                err.offset = base_offset;
                return false;
            }
        }
    }

    return true;
}



/**
 * @brief Split a normalized pattern into segments and parse each segment.
 *
 * This is the entry point for building the segment/atom representation.
 * If the pattern starts with the separator, it is considered absolute.
 *
 * @param norm Normalized pattern string.
 * @param out Output pattern structure (segments are appended).
 * @param err Output parse error.
 * @param opt Parsing options.
 * @return true on success, false on parse error.
 */
static bool SplitSegments(const std::string& norm, Pattern& out, ParseError& err, const Options& opt) noexcept
{
    std::size_t i = 0;

    // DETECT ABSOLUTE PATTERNS
    if (!norm.empty() && norm[0] == opt.separator)
    {
        out.absolute = true;
        i = 1;
    }

    Segment cur{};
    std::size_t seg_start = i;

    // PARSE THE CURRENT SLICE [seg_start, i)
    auto flush_segment = [&] () noexcept -> bool
    {
        std::string_view seg(&norm[seg_start], i - seg_start);

        Segment parsed{};
        if (!ParseSegment(seg, parsed, seg_start, err, opt))
        {
            return false;
        }

        out.segments.push_back(std::move(parsed));
        return true;
    };

    // SPLIT BY SEPARATOR AND PARSE EACH SEGMENT
    for (; i <= norm.size(); ++i)
    {
        if (i == norm.size() || norm[i] == opt.separator)
        {
            if (!flush_segment())
            {
                return false;
            }

            seg_start = i + 1;
        }
    }

    return true;
}




//    ███████╗██╗  ██╗██████╗  █████╗ ███╗   ██╗██████╗ ███████╗██████╗
//    ██╔════╝╚██╗██╔╝██╔══██╗██╔══██╗████╗  ██║██╔══██╗██╔════╝██╔══██╗
//    █████╗   ╚███╔╝ ██████╔╝███████║██╔██╗ ██║██║  ██║█████╗  ██████╔╝
//    ██╔══╝   ██╔██╗ ██╔═══╝ ██╔══██║██║╚██╗██║██║  ██║██╔══╝  ██╔══██╗
//    ███████╗██╔╝ ██╗██║     ██║  ██║██║ ╚████║██████╔╝███████╗██║  ██║
//    ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝  ╚═╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚═╝  ╚═╝




/**
 * @brief Check if a filesystem entry name starts with '.'.
 *
 * Used to implement the common glob rule where dotfiles are excluded unless
 * explicitly requested.
 *
 * @param name Entry filename (single segment).
 * @return true if it begins with '.'.
 */
static bool StartsWithDot(const std::string& name) noexcept
{
    return !name.empty() && name[0] == '.';
}



/**
 * @brief Determine whether a segment allows matching dotfiles implicitly.
 *
 * When dotfiles are excluded, a segment can still match them if it starts
 * with a literal '.' character.
 *
 * @param seg Segment to inspect.
 * @return true if dotfiles can be matched by this segment.
 */
static bool SegmentAllowsDotfiles(const Segment& seg) noexcept
{
    // EMPTY SEGMENT IS TREATED AS ALLOWING DOTFILES
    if (seg.atoms.empty())
    {
        return true;
    }

    // ONLY A LEADING LITERAL '.' ENABLES DOTFILE MATCHING
    const Atom& a0 = seg.atoms[0];
    if (a0.kind == Atom::Kind::LITERAL)
    {
        if (!a0.literal.empty() && a0.literal[0] == '.')
        {
            return true;
        }
    }

    return false;
}



/**
 * @brief Check whether a segment is a single literal atom.
 *
 * Used as a fast-path during expansion to avoid directory enumeration
 * when the next step is a deterministic single child lookup.
 *
 * @param seg Segment to test.
 * @param out_lit Output literal view.
 * @return true if the segment is exactly one literal atom.
 */
static bool SegmentIsLiteralOnly(const Segment& seg, std::string_view& out_lit) noexcept
{
    if (seg.atoms.size() != 1)
    {
        return false;
    }
    if (seg.atoms[0].kind != Atom::Kind::LITERAL)
    {
        return false;
    }

    out_lit = seg.atoms[0].literal;
    return true;
}



/**
 * @brief Match a single character against a parsed character class.
 *
 * The class can be:
 * - A list of single characters
 * - A list of inclusive ranges
 * - Optionally negated
 *
 * @param cc Parsed character class.
 * @param ch Candidate character.
 * @return true if it matches.
 */
static bool CharClassMatch(const CharClass& cc, char ch) noexcept
{
    bool hit = false;

    // CHECK SINGLES FIRST
    for (char c : cc.singles)
    {
        if (c == ch)
        {
            hit = true;
            break;
        }
    }

    // CHECK RANGES IF NOT MATCHED
    if (!hit)
    {
        for (const auto& r : cc.ranges)
        {
            unsigned char a = static_cast<unsigned char>(r.first);
            unsigned char b = static_cast<unsigned char>(r.last);
            unsigned char x = static_cast<unsigned char>(ch);

            if (x >= a && x <= b)
            {
                hit = true;
                break;
            }
        }
    }

    // APPLY NEGATION
    if (cc.negated)
    {
        return !hit;
    }

    return hit;
}



/**
 * @brief Match a segment against a single filename segment.
 *
 * This implements matching for segment-local atoms using a DP table:
 * - LITERAL: match exact substring
 * - QMARK: match exactly one character
 * - STAR: match any sequence (including empty) within this segment
 * - CHARCLASS: match exactly one character if in the class
 *
 * @param seg Parsed segment (no separators).
 * @param name Candidate name (single path segment).
 * @return true if the segment matches the name.
 */
static bool MatchSegmentAtoms(const Segment& seg, const std::string& name) noexcept
{
    // FAST-PATHS FOR COMMON SEGMENT SHAPES (AVOID DP)
    // NOTE: DOTFILE POLICY IS HANDLED BY THE CALLER.

    auto starts_with = [] (std::string_view s, std::string_view pre) noexcept -> bool
    {
        if (s.size() < pre.size())
        {
            return false;
        }
        return s.substr(0, pre.size()) == pre;
    };

    auto ends_with = [] (std::string_view s, std::string_view suf) noexcept -> bool
    {
        if (s.size() < suf.size())
        {
            return false;
        }
        return s.substr(s.size() - suf.size(), suf.size()) == suf;
    };

    const std::size_t Afast = seg.atoms.size();
    if (Afast == 0)
    {
        return name.empty();
    }

    // SINGLE ATOM CASES
    if (Afast == 1)
    {
        const Atom& a0 = seg.atoms[0];

        if (a0.kind == Atom::Kind::LITERAL)
        {
            return name == a0.literal;
        }
        if (a0.kind == Atom::Kind::STAR)
        {
            return true;
        }
        if (a0.kind == Atom::Kind::QMARK)
        {
            return name.size() == 1;
        }
        if (a0.kind == Atom::Kind::CHARCLASS)
        {
            return (name.size() == 1) && CharClassMatch(a0.cls, name[0]);
        }
    }

    // PURE QMARKS: EXACT LENGTH MATCH
    bool only_qmarks = true;
    for (const auto& a : seg.atoms)
    {
        if (a.kind != Atom::Kind::QMARK)
        {
            only_qmarks = false;
            break;
        }
    }
    if (only_qmarks)
    {
        return name.size() == seg.atoms.size();
    }

    // SIMPLE STAR FORMS WITHOUT CHARCLASSES:
    //   "*.ext"   => STAR + LITERAL
    //   "pre*"    => LITERAL + STAR
    //   "pre*suf" => LITERAL + STAR + LITERAL
    //   "*suf"    => STAR + LITERAL
    //   "pre*"    => LITERAL + STAR
    bool has_charclass = false;
    for (const auto& a : seg.atoms)
    {
        if (a.kind == Atom::Kind::CHARCLASS)
        {
            has_charclass = true;
            break;
        }
    }

    if (!has_charclass)
    {
        if (Afast == 2 && seg.atoms[0].kind == Atom::Kind::STAR && seg.atoms[1].kind == Atom::Kind::LITERAL)
        {
            return ends_with(name, seg.atoms[1].literal);
        }
        if (Afast == 2 && seg.atoms[0].kind == Atom::Kind::LITERAL && seg.atoms[1].kind == Atom::Kind::STAR)
        {
            return starts_with(name, seg.atoms[0].literal);
        }
        if (Afast == 3 && seg.atoms[0].kind == Atom::Kind::LITERAL && seg.atoms[1].kind == Atom::Kind::STAR && seg.atoms[2].kind == Atom::Kind::LITERAL)
        {
            const std::string& pre = seg.atoms[0].literal;
            const std::string& suf = seg.atoms[2].literal;

            if (name.size() < pre.size() + suf.size())
            {
                return false;
            }

            return starts_with(name, pre) && ends_with(name, suf);
        }
    }

    const std::size_t A = seg.atoms.size();
    const std::size_t N = name.size();

    // DP[i][j] = true IF FIRST i ATOMS MATCH FIRST j CHARACTERS
    // USE A ROLLING DP TO AVOID HEAP CHURN FROM vector<vector<bool>>.
    std::vector<std::uint8_t> cur(N + 1, 0);
    std::vector<std::uint8_t> nxt(N + 1, 0);
    cur[0] = 1;

    // ADVANCE OVER ATOMS
    for (std::size_t i = 0; i < A; ++i)
    {
        const Atom& a = seg.atoms[i];

        std::fill(nxt.begin(), nxt.end(), 0);

        if (a.kind == Atom::Kind::LITERAL)
        {
            // LITERAL: CONSUME EXACT SUBSTRING
            const std::string& lit = a.literal;
            for (std::size_t j = 0; j <= N; ++j)
            {
                if (!cur[j])
                {
                    continue;
                }

                if (j + lit.size() <= N && name.compare(j, lit.size(), lit) == 0)
                {
                    nxt[j + lit.size()] = 1;
                }
            }
        }
        else if (a.kind == Atom::Kind::QMARK)
        {
            // QMARK: MATCH EXACTLY ONE CHAR
            for (std::size_t j = 0; j < N; ++j)
            {
                if (cur[j])
                {
                    nxt[j + 1] = 1;
                }
            }
        }
        else if (a.kind == Atom::Kind::STAR)
        {
            // STAR: MATCH 0..N CHARS WITHIN THE SEGMENT
            // OPTIMIZED STAR TRANSITION:
            // If any cur[j] is true, then all nxt[k] for k>=j become true.
            // Implement as a prefix scan from left-to-right.
            std::uint8_t seen = 0;
            for (std::size_t j = 0; j <= N; ++j)
            {
                if (cur[j])
                {
                    seen = 1;
                }
                if (seen)
                {
                    nxt[j] = 1;
                }
            }
        }
        else if (a.kind == Atom::Kind::CHARCLASS)
        {
            // CHARCLASS: MATCH ONE CHAR IF IN CLASS
            for (std::size_t j = 0; j < N; ++j)
            {
                if (!cur[j])
                {
                    continue;
                }

                if (CharClassMatch(a.cls, name[j]))
                {
                    nxt[j + 1] = 1;
                }
            }
        }
        else
        {
            // DOUBLESTAR IS NOT VALID INSIDE NORMAL SEGMENTS IN THIS REPRESENTATION
            return false;
        }

        cur.swap(nxt);
    }

    // FULL MATCH REQUIRES CONSUMING ALL ATOMS AND ALL CHARACTERS
    return cur[N] != 0;
}



/**
 * @brief List directory entries with deterministic ordering.
 *
 * The entries are collected into @p entries and sorted by filename (generic string)
 * to keep expansion deterministic across platforms/filesystems.
 *
 * @param dir Directory path to list.
 * @param entries Output vector of directory entries.
 */
static void ListDir(const fs::path& dir, std::vector<fs::directory_entry>& entries) noexcept
{
    // RESET OUTPUT
    entries.clear();

    std::error_code ec;
    fs::directory_iterator it(dir, ec);
    if (ec)
    {
        return;
    }

    // COLLECT ENTRIES
    for (const auto& de : it)
    {
        entries.push_back(de);
    }

    // SORT FOR DETERMINISTIC OUTPUT
    std::sort(entries.begin(), entries.end(),
                [] (const fs::directory_entry& a, const fs::directory_entry& b) {
                    return a.path().filename().generic_string() < b.path().filename().generic_string();
                });
}



/**
 * @brief Determine whether a directory_entry should be treated as a directory.
 *
 * This optionally follows symlinks. When symlink following is disabled, symlinks
 * are treated conservatively (symlink-to-dir does not count as a directory).
 *
 * @param de Directory entry to test.
 * @param follow_symlinks Whether symlinks should be followed.
 * @return true if the entry is a directory per the chosen policy.
 */
static bool IsDir(const fs::directory_entry& de, bool follow_symlinks) noexcept
{
    std::error_code ec;

    // FOLLOW SYMLINKS IF REQUESTED
    if (follow_symlinks)
    {
        return de.is_directory(ec);
    }

    // CONSERVATIVE MODE: DO NOT TREAT SYMLINKS AS DIRECTORIES
    if (de.is_symlink(ec))
    {
        return false;
    }

    return de.is_directory(ec);
}



/**
 * @brief Recursive filesystem expansion of a parsed glob pattern.
 *
 * The algorithm walks pattern segments while descending the filesystem:
 * - For a DOUBLESTAR segment, it tries the 0-directory case and then recurses into each subdirectory
 *   reusing the same segment index (to match multiple levels).
 * - For a normal segment, it lists the current directory, matches entries against the segment atoms,
 *   and then advances to the next segment.
 *
 * @param pattern Parsed glob pattern.
 * @param opt Expansion options (dotfiles policy, symlink policy).
 * @param cur_dir Current directory in the traversal.
 * @param seg_index Index of the current pattern segment.
 * @param out Output list of matched generic paths.
 */
static void ExpandRec(const Pattern& pattern, const ExpandOptions& opt, const fs::path& cur_dir, std::size_t seg_index, std::vector<std::string>& out) noexcept
{
    // TERMINATION: ALL SEGMENTS CONSUMED
    if (seg_index >= pattern.segments.size())
    {
        // EMIT FINAL MATCH PATH
        out.push_back(cur_dir.lexically_normal().generic_string());
        return;
    }

    const Segment& seg = pattern.segments[seg_index];

    // HANDLE DOUBLESTAR SEGMENT: 0..N DIRECTORIES
    if (seg.IsDoubleStarOnly())
    {
        // TRY 0-DIRECTORY MATCH (ADVANCE PATTERN WITHOUT DESCENT)
        ExpandRec(pattern, opt, cur_dir, seg_index + 1, out);

        // TRY N-DIRECTORY MATCHES (DESCEND INTO EACH SUBDIR AND REUSE SEG_INDEX)
        std::vector<fs::directory_entry> entries;
        ListDir(cur_dir, entries);

        for (const auto& de : entries)
        {
            std::string name = de.path().filename().generic_string();

            // APPLY DOTFILE FILTER AT DIRECTORY TRAVERSAL TIME
            if (!opt.include_dotfiles && StartsWithDot(name))
            {
                continue;
            }

            // ONLY DESCEND INTO DIRECTORIES UNDER THE SELECTED POLICY
            if (!IsDir(de, opt.follow_symlinks))
            {
                continue;
            }

            ExpandRec(pattern, opt, de.path(), seg_index, out);
        }

        return;
    }

    // FAST-PATH: LITERAL-ONLY SEGMENT CAN BE RESOLVED WITHOUT ENUMERATION.
    // THIS IS SEMANTICALLY IDENTICAL TO LISTDIR+MATCH BUT AVOIDS SCANNING.
    {
        std::string_view lit;
        if (SegmentIsLiteralOnly(seg, lit))
        {
            // DOTFILES POLICY: A LEADING '.' IS EXPLICIT, SO IT IS ALLOWED
            // EVEN WHEN include_dotfiles IS FALSE.
            std::error_code ec;
            fs::path next = cur_dir / fs::path(std::string(lit));

            if (!fs::exists(next, ec))
            {
                return;
            }

            // IF THERE ARE MORE SEGMENTS, WE MUST DESCEND INTO A DIRECTORY
            if (seg_index + 1 < pattern.segments.size())
            {
                if (!fs::is_directory(next, ec))
                {
                    return;
                }
            }

            ExpandRec(pattern, opt, next, seg_index + 1, out);
            return;
        }
    }

    // NORMAL SEGMENT: MATCH IN CURRENT DIRECTORY AND ADVANCE
    std::vector<fs::directory_entry> entries;
    ListDir(cur_dir, entries);

    // DOTFILES ARE ALLOWED IF EXPLICITLY ENABLED OR THE SEGMENT LEADS WITH '.'
    bool allow_dot = opt.include_dotfiles || SegmentAllowsDotfiles(seg);

    for (const auto& de : entries)
    {
        std::string name = de.path().filename().generic_string();

        // FILTER DOTFILES WHEN NOT ALLOWED
        if (!allow_dot && StartsWithDot(name))
        {
            continue;
        }

        // MATCH THIS SEGMENT AGAINST THE ENTRY NAME
        if (!MatchSegmentAtoms(seg, name))
        {
            continue;
        }

        // IF THERE ARE MORE SEGMENTS, WE MUST DESCEND INTO A DIRECTORY
        if (seg_index + 1 < pattern.segments.size())
        {
            if (!IsDir(de, opt.follow_symlinks))
            {
                continue;
            }
        }

        // RECURSE TO NEXT SEGMENT
        ExpandRec(pattern, opt, de.path(), seg_index + 1, out);
    }
}





//     ██████╗ ██╗      ██████╗ ██████╗     ██████╗      ██████╗ ██╗      ██████╗ ██████╗
//    ██╔════╝ ██║     ██╔═══██╗██╔══██╗    ╚════██╗    ██╔════╝ ██║     ██╔═══██╗██╔══██╗
//    ██║  ███╗██║     ██║   ██║██████╔╝     █████╔╝    ██║  ███╗██║     ██║   ██║██████╔╝
//    ██║   ██║██║     ██║   ██║██╔══██╗    ██╔═══╝     ██║   ██║██║     ██║   ██║██╔══██╗
//    ╚██████╔╝███████╗╚██████╔╝██████╔╝    ███████╗    ╚██████╔╝███████╗╚██████╔╝██████╔╝
//     ╚═════╝ ╚══════╝ ╚═════╝ ╚═════╝     ╚══════╝     ╚═════╝ ╚══════╝ ╚═════╝ ╚═════╝




/**
 * @brief Split a generic path string into segment views.
 *
 * This is used by the glob-to-glob mapping logic, where source paths are treated
 * as generic ('/') separated paths independent of platform.
 *
 * @param s Input path.
 * @param segs Output list of non-empty segment views.
 */
static void SplitPathSegments(std::string_view s, std::vector<std::string_view>& segs) noexcept
{
    // RESET OUTPUT
    segs.clear();

    // SCAN AND SLICE BY '/'
    std::size_t start = 0;
    for (std::size_t i = 0; i <= s.size(); ++i)
    {
        if (i == s.size() || s[i] == '/')
        {
            if (i > start)
            {
                segs.push_back(s.substr(start, i - start));
            }
            start = i + 1;
        }
    }
}



/**
 * @brief Join a subrange of path segments into a generic path string.
 *
 * This is primarily used to materialize PATH captures for DOUBLESTAR segments.
 *
 * @param segs Source segments.
 * @param from Start index (inclusive).
 * @param to End index (exclusive).
 * @return Joined generic path.
 */
static std::string JoinPathSegments(const std::vector<std::string_view>& segs,
                                    std::size_t                         from,
                                    std::size_t                         to) noexcept
{
    std::string out;

    // CONCATENATE WITH '/' BETWEEN SEGMENTS
    for (std::size_t i = from; i < to; ++i)
    {
        if (i != from)
        {
            out.push_back('/');
        }
        out.append(segs[i].data(), segs[i].size());
    }

    return out;
}



/**
 * @brief DP predecessor metadata for segment-matching with captures.
 *
 * This structure is used to reconstruct capture slices during backtracking.
 */
struct PrevCell
{
    bool        has_prev = false;
    std::size_t pi       = 0;
    std::size_t pj       = 0;

    bool        capture  = false;
    Capture::Kind kind   = Capture::Kind::SEGMENT;
    std::size_t  cs      = 0;
    std::size_t  ce      = 0;
};



/**
 * @brief Match a segment against a name while producing capture tokens.
 *
 * Captures are emitted for:
 * - QMARK and CHARCLASS: a single character (CHAR capture)
 * - STAR: a variable-length substring within the segment (SEGMENT capture)
 *
 * The algorithm:
 * - Runs a DP like MatchSegmentAtoms but keeps a predecessor table to backtrack.
 * - Uses deterministic predecessor selection (first time a state is reached wins).
 *
 * @param seg Parsed segment.
 * @param name Candidate segment name.
 * @param out_caps Output captures in forward order.
 * @return true if the segment matches and captures were produced, false otherwise.
 */
static bool MatchSegmentCapture(const Segment&           seg,
                                std::string_view         name,
                                std::vector<Capture>&    out_caps) noexcept
{
    // RESET OUTPUT
    out_caps.clear();

    const std::size_t A = seg.atoms.size();
    const std::size_t N = name.size();

    // DP + PREDECESSOR TABLE
    std::vector<std::vector<bool>>     dp(A + 1, std::vector<bool>(N + 1, false));
    std::vector<std::vector<PrevCell>> prev(A + 1, std::vector<PrevCell>(N + 1));

    // BASE STATE
    dp[0][0] = true;
    prev[0][0].has_prev = true;
    prev[0][0].pi = 0;
    prev[0][0].pj = 0;

    // ADVANCE OVER ATOMS
    for (std::size_t i = 0; i < A; ++i)
    {
        const Atom& a = seg.atoms[i];

        // DOUBLESTAR IS NOT ALLOWED INSIDE A NORMAL SEGMENT
        if (a.kind == Atom::Kind::DOUBLESTAR)
        {
            return false;
        }

        if (a.kind == Atom::Kind::LITERAL)
        {
            // LITERAL: NO CAPTURE, JUST ADVANCE IF IT MATCHES
            const std::string& lit = a.literal;

            for (std::size_t j = 0; j <= N; ++j)
            {
                if (!dp[i][j])
                {
                    continue;
                }

                if (j + lit.size() <= N && name.substr(j, lit.size()) == lit)
                {
                    if (!dp[i + 1][j + lit.size()])
                    {
                        dp[i + 1][j + lit.size()] = true;

                        prev[i + 1][j + lit.size()].has_prev = true;
                        prev[i + 1][j + lit.size()].pi = i;
                        prev[i + 1][j + lit.size()].pj = j;
                    }
                }
            }
        }
        else if (a.kind == Atom::Kind::QMARK)
        {
            // QMARK: CAPTURE ONE CHAR
            for (std::size_t j = 0; j < N; ++j)
            {
                if (!dp[i][j])
                {
                    continue;
                }

                if (!dp[i + 1][j + 1])
                {
                    dp[i + 1][j + 1] = true;

                    PrevCell& p = prev[i + 1][j + 1];
                    p.has_prev = true;
                    p.pi = i;
                    p.pj = j;

                    p.capture = true;
                    p.kind = Capture::Kind::CHAR;
                    p.cs = j;
                    p.ce = j + 1;
                }
            }
        }
        else if (a.kind == Atom::Kind::CHARCLASS)
        {
            // CHARCLASS: CAPTURE ONE CHAR IF IT MATCHES
            for (std::size_t j = 0; j < N; ++j)
            {
                if (!dp[i][j])
                {
                    continue;
                }

                if (!CharClassMatch(a.cls, name[j]))
                {
                    continue;
                }

                if (!dp[i + 1][j + 1])
                {
                    dp[i + 1][j + 1] = true;

                    PrevCell& p = prev[i + 1][j + 1];
                    p.has_prev = true;
                    p.pi = i;
                    p.pj = j;

                    p.capture = true;
                    p.kind = Capture::Kind::CHAR;
                    p.cs = j;
                    p.ce = j + 1;
                }
            }
        }
        else if (a.kind == Atom::Kind::STAR)
        {
            // STAR: CAPTURE A SUBSTRING (INCLUDING EMPTY)
            for (std::size_t j = 0; j <= N; ++j)
            {
                if (!dp[i][j])
                {
                    continue;
                }

                // DETERMINISTIC: FIRST TIME A STATE IS REACHED WINS
                for (std::size_t k = j; k <= N; ++k)
                {
                    if (!dp[i + 1][k])
                    {
                        dp[i + 1][k] = true;

                        PrevCell& p = prev[i + 1][k];
                        p.has_prev = true;
                        p.pi = i;
                        p.pj = j;

                        p.capture = true;
                        p.kind = Capture::Kind::SEGMENT;
                        p.cs = j;
                        p.ce = k;
                    }
                }
            }
        }
    }

    // REQUIRE FULL MATCH
    if (!dp[A][N])
    {
        return false;
    }

    // BACKTRACK CAPTURES IN REVERSE ORDER
    std::vector<Capture> rev;

    std::size_t ci = A;
    std::size_t cj = N;

    while (!(ci == 0 && cj == 0))
    {
        const PrevCell& p = prev[ci][cj];
        if (!p.has_prev)
        {
            return false;
        }

        if (p.capture)
        {
            Capture cap{};
            cap.kind = p.kind;
            cap.value.assign(name.substr(p.cs, p.ce - p.cs));
            rev.push_back(std::move(cap));
        }

        ci = p.pi;
        cj = p.pj;
    }

    // REVERSE TO FORWARD ORDER
    std::reverse(rev.begin(), rev.end());
    out_caps = std::move(rev);
    return true;
}



/**
 * @brief Recursive matcher for pattern-to-path with capture generation.
 *
 * This matches a parsed "from" pattern against a source generic path split into segments.
 * It supports multiple DOUBLESTAR segments and produces captures in the order required
 * by the instantiation logic.
 *
 * memo_fail is used as a visited-failure table:
 * - memo_fail[pi][si] == 0 means "this (pattern index, src index) state already failed".
 *
 * @param from_pat Parsed pattern being matched.
 * @param src_segs Source path segments.
 * @param pi Current pattern segment index.
 * @param si Current source segment index.
 * @param caps Capture accumulator.
 * @param memo_fail Failure memo table.
 * @return true if a match is found, false otherwise.
 */
static bool MatchCaptureRec(const Pattern&                        from_pat,
                            const std::vector<std::string_view>&   src_segs,
                            std::size_t                            pi,
                            std::size_t                            si,
                            std::vector<Capture>&                  caps,
                            std::vector<std::vector<std::int8_t>>& memo_fail) noexcept
{
    // TERMINATION: ALL PATTERN SEGMENTS CONSUMED
    if (pi == from_pat.segments.size())
    {
        return si == src_segs.size();
    }

    // GUARD AGAINST OVERRUN
    if (si > src_segs.size())
    {
        return false;
    }

    // FAIL-FAST IF THIS STATE WAS ALREADY PROVEN IMPOSSIBLE
    if (memo_fail[pi][si] == 0)
    {
        return false;
    }

    const Segment& seg = from_pat.segments[pi];

    // DOUBLESTAR: CAPTURE 0..N SOURCE SEGMENTS AS A PATH
    if (seg.IsDoubleStarOnly())
    {
        // TRY SHORTEST MATCH FIRST FOR DETERMINISM
        for (std::size_t t = si; t <= src_segs.size(); ++t)
        {
            Capture c{};
            c.kind = Capture::Kind::PATH;
            c.value = JoinPathSegments(src_segs, si, t);

            caps.push_back(std::move(c));

            if (MatchCaptureRec(from_pat, src_segs, pi + 1, t, caps, memo_fail))
            {
                return true;
            }

            caps.pop_back();
        }

        // MEMOIZE FAILURE
        memo_fail[pi][si] = 0;
        return false;
    }

    // NON-DOUBLESTAR: MUST HAVE A SOURCE SEGMENT AVAILABLE
    if (si >= src_segs.size())
    {
        memo_fail[pi][si] = 0;
        return false;
    }

    std::vector<Capture> local;

    // MATCH ONE SEGMENT AND PRODUCE LOCAL CAPTURES
    if (!MatchSegmentCapture(seg, src_segs[si], local))
    {
        memo_fail[pi][si] = 0;
        return false;
    }

    // APPEND CAPTURES, RECURSE, AND ROLLBACK ON FAILURE
    const std::size_t old_size = caps.size();
    for (auto& c : local)
    {
        caps.push_back(std::move(c));
    }

    if (MatchCaptureRec(from_pat, src_segs, pi + 1, si + 1, caps, memo_fail))
    {
        return true;
    }

    // ROLLBACK AND MEMOIZE FAILURE
    caps.resize(old_size);
    memo_fail[pi][si] = 0;
    return false;
}



/**
 * @brief Match a "from" pattern against a generic path and collect captures.
 *
 * @param from_pat Parsed pattern used for matching.
 * @param src_generic Source path as a generic string ('/' separators).
 * @param caps Output captures in forward order.
 * @return true on match, false otherwise.
 */
static bool MatchCapture(const Pattern&            from_pat,
                    std::string_view          src_generic,
                    std::vector<Capture>&     caps) noexcept
{
    // RESET OUTPUT
    caps.clear();

    // SPLIT SOURCE INTO SEGMENTS
    std::vector<std::string_view> src_segs;
    SplitPathSegments(src_generic, src_segs);

    // INITIALIZE FAILURE MEMO TABLE
    std::vector<std::vector<std::int8_t>> memo_fail(
        from_pat.segments.size() + 1,
        std::vector<std::int8_t>(src_segs.size() + 1, -1)
    );

    // RUN RECURSIVE MATCHER
    return MatchCaptureRec(from_pat, src_segs, 0, 0, caps, memo_fail);
}



/**
 * @brief Check whether a capture is of the expected kind.
 *
 * @param c Capture to test.
 * @param k Required capture kind.
 * @return true if kinds match.
 */
static bool Consume(const Capture& c, Capture::Kind k) noexcept
{
    return c.kind == k;
}



/**
 * @brief Instantiate a "to" pattern by consuming captures produced by MatchCapture().
 *
 * This rebuilds an output generic path by walking the "to" pattern segments:
 * - DOUBLESTAR consumes a PATH capture and injects its subsegments as whole segments.
 * - STAR consumes a SEGMENT capture and injects it into the current segment.
 * - QMARK/CHARCLASS consume CHAR captures and inject exactly one character each.
 * - LITERAL atoms are copied verbatim.
 *
 * @param to_pat Parsed pattern used for instantiation.
 * @param caps Captures produced during matching.
 * @param out_generic Output instantiated generic path.
 * @return true on success, false if pattern/capture sequence are incompatible.
 */
static bool Instantiate(const Pattern&              to_pat,
                    const std::vector<Capture>& caps,
                    std::string&                out_generic) noexcept
{
    // RESET OUTPUT
    out_generic.clear();

    std::size_t cap_i = 0;
    std::vector<std::string> out_segs;

    // WALK OUTPUT PATTERN SEGMENTS
    for (const auto& seg : to_pat.segments)
    {
        if (seg.IsDoubleStarOnly())
        {
            // CONSUME A PATH CAPTURE AND SPLIT IT INTO OUTPUT SEGMENTS
            if (cap_i >= caps.size())
            {
                return false;
            }
            if (!Consume(caps[cap_i], Capture::Kind::PATH))
            {
                return false;
            }

            std::vector<std::string_view> mid;
            SplitPathSegments(caps[cap_i].value, mid);

            for (const auto& s : mid)
            {
                out_segs.emplace_back(std::string(s));
            }

            ++cap_i;
            continue;
        }

        std::string built;

        // BUILD A SINGLE OUTPUT SEGMENT FROM ATOMS
        for (const auto& a : seg.atoms)
        {
            if (a.kind == Atom::Kind::LITERAL)
            {
                built += a.literal;
                continue;
            }

            if (cap_i >= caps.size())
            {
                return false;
            }

            if (a.kind == Atom::Kind::STAR)
            {
                // CONSUME SEGMENT CAPTURE
                if (!Consume(caps[cap_i], Capture::Kind::SEGMENT))
                {
                    return false;
                }
                built += caps[cap_i].value;
                ++cap_i;
                continue;
            }

            if (a.kind == Atom::Kind::QMARK || a.kind == Atom::Kind::CHARCLASS)
            {
                // CONSUME CHAR CAPTURE
                if (!Consume(caps[cap_i], Capture::Kind::CHAR))
                {
                    return false;
                }
                if (caps[cap_i].value.size() != 1)
                {
                    return false;
                }
                built += caps[cap_i].value;
                ++cap_i;
                continue;
            }

            // DOUBLESTAR SHOULD NOT APPEAR HERE (ONLY SEGMENT-ONLY)
            return false;
        }

        out_segs.push_back(std::move(built));
    }

    // ALL CAPTURES MUST BE CONSUMED
    if (cap_i != caps.size())
    {
        return false;
    }

    // JOIN SEGMENTS INTO A GENERIC PATH
    for (std::size_t i = 0; i < out_segs.size(); ++i)
    {
        if (i != 0)
        {
            out_generic.push_back('/');
        }
        out_generic += out_segs[i];
    }

    return true;
}





//    ███╗   ███╗ ██████╗ ██████╗ ██╗   ██╗██╗     ███████╗    ██╗███╗   ███╗██████╗ ██╗
//    ████╗ ████║██╔═══██╗██╔══██╗██║   ██║██║     ██╔════╝    ██║████╗ ████║██╔══██╗██║
//    ██╔████╔██║██║   ██║██║  ██║██║   ██║██║     █████╗      ██║██╔████╔██║██████╔╝██║
//    ██║╚██╔╝██║██║   ██║██║  ██║██║   ██║██║     ██╔══╝      ██║██║╚██╔╝██║██╔═══╝ ██║
//    ██║ ╚═╝ ██║╚██████╔╝██████╔╝╚██████╔╝███████╗███████╗    ██║██║ ╚═╝ ██║██║     ███████╗
//    ╚═╝     ╚═╝ ╚═════╝ ╚═════╝  ╚═════╝ ╚══════╝╚══════╝    ╚═╝╚═╝     ╚═╝╚═╝     ╚══════╝




/**
 * @brief Parse a glob string into a Pattern object.
 *
 * This function:
 * - Validates the input is non-empty.
 * - Normalizes separators according to options.
 * - Splits into segments and parses each segment into atoms.
 *
 * @param input Raw glob pattern.
 * @param out Output pattern.
 * @param err Output parse error.
 * @param opt Parsing options.
 * @return true on success, false on parse error.
 */
bool Arcb::Glob::Parse(std::string_view input, Pattern& out, ParseError& err, const Options& opt) noexcept
{
    // RESET OUTPUT STRUCTURES
    out = Pattern{};
    err = ParseError{};

    // VALIDATE INPUT
    if (input.size() == 0)
    {
        err.code   = ParseError::Code::EMPTY_PATTERN;
        err.offset = 0;
        return false;
    }

    // NORMALIZE INPUT STRING
    out.normalized = Normalize(input, opt);

    // SPLIT AND PARSE SEGMENTS
    if (!SplitSegments(out.normalized, out, err, opt))
    {
        return false;
    }

    return true;
}




/**
 * @brief Expand a parsed pattern against the filesystem.
 *
 * The expansion:
 * - Chooses a starting directory depending on whether the pattern is absolute.
 * - Recursively enumerates directory entries and applies segment matching.
 * - Sorts and deduplicates results for deterministic output.
 *
 * @param pattern Parsed pattern to expand.
 * @param base_dir Base directory used for relative patterns.
 * @param out Output list of matched generic paths.
 * @param opt Expansion options.
 * @return true if expansion ran (start path exists), false if start path does not exist.
 */
bool Arcb::Glob::Expand(const Pattern& pattern, const fs::path& base_dir, std::vector<std::string>& out, const ExpandOptions& opt) noexcept
{
    std::error_code ec;

    // SELECT START DIRECTORY
    fs::path start = base_dir;
    if (pattern.absolute)
    {
        // RESOLVE ROOT DIRECTORY FOR ABSOLUTE PATTERNS
        fs::path root = base_dir.root_path();
        if (root.empty())
        {
            root = fs::path("/");
        }
        start = root;
    }

    // FAIL IF START DOES NOT EXIST
    if (!fs::exists(start, ec))
    {
        return false;
    }

    // RUN RECURSIVE EXPANSION
    ExpandRec(pattern, opt, start, 0, out);

    // SORT AND DEDUP FOR DETERMINISTIC OUTPUT
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());

    return true;
}



/**
 * @brief Map a list of paths from one glob "shape" into another.
 *
 * This function:
 * - Parses the source ("from") and destination ("to") globs.
 * - For each source string, captures wildcard matches using the "from" pattern.
 * - Instantiates the "to" pattern using the produced captures.
 *
 * Errors are reported through:
 * - err_from for parsing issues in from_glob,
 * - err_to for parsing issues in to_glob,
 * - err_map for capture/instantiate failures on specific sources.
 *
 * @param from_glob Glob used to capture wildcards from source paths.
 * @param to_glob Glob used to build destination paths.
 * @param src_list List of source generic paths.
 * @param out_list Output mapped paths.
 * @param err_from Parse error for from_glob.
 * @param err_to Parse error for to_glob.
 * @param err_map Mapping error (capture/instantiate).
 * @return true on success, false on parse or mapping failure.
 */
bool Arcb::Glob:: MapGlobToGlob(std::vector<std::string>  from_glob,
                    std::string_view  to_glob,
                    std::vector<std::string> src_list,
                    std::vector<std::string>&       out_list,
                    ParseError&       err_from,
                    ParseError&       err_to,
                    MapError&         err_map) noexcept
{
    // RESET OUTPUT LIST
    out_list.clear();

    // PARSE DESTINATION GLOB ONCE (CONSTANT ACROSS MAPPINGS)
    Pattern to_pat;
    if (!Glob::Parse(to_glob, to_pat, err_to))
    {
        return false;
    }

    for (uint32_t i = 0; i < from_glob.size(); ++i)
    {
        const auto& from = from_glob[i];

        Pattern from_pat;
    
        // PARSE SOURCE GLOB
        if (!Glob::Parse(from, from_pat, err_from))
        {
            return false;
        }
    
        // MAP EACH SOURCE PATH THROUGH CAPTURE + INSTANTIATE
        for (auto it = src_list.begin(); it != src_list.end();)
        {
            std::vector<Arcb::Glob::Capture> caps;
            if (!MatchCapture(from_pat, *it, caps))
            {
                if (i == from_glob.size() - 1)
                {
                    err_map.code = MapError::Code::CAPTURE;
                    return false;
                } 
                ++it;
                continue;
            }
    
            std::string out;
            if (!Instantiate(to_pat, caps, out))
            {
                err_map.code = MapError::Code::INSTANTIATE;
                return false;
            }
            
            it = src_list.erase(it);

            out_list.emplace_back(std::move(out));
        }
    }

    return true;
}
