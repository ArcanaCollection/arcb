#include "Cache.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <functional>

USE_MODULE(Arcb::Cache);

#define _P(_path) (fs::path(_path))





enum CacheType : std::uint32_t
{
    CT__PROFILE = 0,
    CT__FILES   = 16
};




//    ███████╗███████╗    ██╗  ██╗███████╗██╗     ██████╗ ███████╗██████╗ 
//    ██╔════╝██╔════╝    ██║  ██║██╔════╝██║     ██╔══██╗██╔════╝██╔══██╗
//    █████╗  ███████╗    ███████║█████╗  ██║     ██████╔╝█████╗  ██████╔╝
//    ██╔══╝  ╚════██║    ██╔══██║██╔══╝  ██║     ██╔═══╝ ██╔══╝  ██╔══██╗
//    ██║     ███████║    ██║  ██║███████╗███████╗██║     ███████╗██║  ██║
//    ╚═╝     ╚══════╝    ╚═╝  ╚═╝╚══════╝╚══════╝╚═╝     ╚══════╝╚═╝  ╚═╝
//                                                                        

/**
 * @brief Check if a directory exists.
 * @param p Path to check.
 * @return True if `p` exists and is a directory.
 */
inline bool dir_exists(const fs::path& p) noexcept
{
    std::error_code ec;
    return fs::exists(p, ec) && fs::is_directory(p, ec);
}

/**
 * @brief Check if a regular file exists.
 * @param p Path to check.
 * @return True if `p` exists and is a regular file.
 */
inline bool file_exists(const fs::path& p) noexcept
{
    std::error_code ec;
    return fs::exists(p, ec) && fs::is_regular_file(p, ec);
}

/**
 * @brief Create a directory tree if missing.
 * @param p Directory path.
 * @return True on success (or if already exists as directory).
 */
inline bool create_dir(const fs::path& p) noexcept
{
    std::error_code ec;

    if (fs::exists(p, ec))
    {
        return fs::is_directory(p, ec);
    }

    return fs::create_directories(p, ec);
}

/**
 * @brief Create (or overwrite) a file with content, creating parent dirs if needed.
 * @param p Target file path.
 * @param content Bytes to write.
 * @return True on success.
 */
inline bool create_file(const fs::path& p, const std::string& content) noexcept
{
    std::error_code ec;

    if (!p.parent_path().empty())
    {
        if (!fs::exists(p.parent_path(), ec))
        {
            if (!fs::create_directories(p.parent_path(), ec))
            {
                return false;
            }
        }
    }

    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        return false;
    }

    out << content;
    return out.good();
}

/**
 * @brief Remove a directory (non-recursive).
 * @param p Directory path.
 * @return True on success.
 */
inline bool remove_dir(const fs::path& p) noexcept
{
    std::error_code ec;

    if (!fs::exists(p, ec))
    {
        return false;
    }

    if (!fs::is_directory(p, ec))
    {
        return false;
    }

    return fs::remove(p, ec);
}

/**
 * @brief Remove a file.
 * @param p File path.
 * @return True on success.
 */
inline bool remove_file(const fs::path& p) noexcept
{
    std::error_code ec;

    if (!fs::exists(p, ec))
    {
        return false;
    }

    if (!fs::is_regular_file(p, ec))
    {
        return false;
    }

    return fs::remove(p, ec);
}

/**
 * @brief Remove a directory recursively.
 * @param p Directory path.
 * @return True on success.
 */
inline bool remove_dir_recursive(const fs::path& p) noexcept
{
    std::error_code ec;

    if (!fs::exists(p, ec))
    {
        return false;
    }

    if (!fs::is_directory(p, ec))
    {
        return false;
    }

    fs::remove_all(p, ec);
    return !ec;
}

/**
 * @brief Iterate all regular files in a directory and invoke callback with their content.
 * @param dir Directory to scan.
 * @param callback Callback invoked for each regular file.
 * @return True if the iteration succeeds, false on directory errors.
 */
inline bool for_each_file(
    const fs::path& dir,
    const std::function<void(const fs::path& filepath,
                             const std::string& filename,
                             const std::string& content)>& callback) noexcept
{
    std::error_code ec;

    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec))
    {
        return false;
    }

    for (const auto& entry : fs::directory_iterator(dir, ec))
    {
        if (ec)
        {
            return false;
        }

        if (!entry.is_regular_file(ec))
        {
            continue;
        }

        const fs::path& p = entry.path();
        std::string filename = p.filename().string();

        // READ FILE CONTENT AS BINARY
        std::ifstream in(p, std::ios::binary);
        if (!in)
        {
            continue;
        }

        std::string content;
        in.seekg(0, std::ios::end);

        std::streampos size = in.tellg();
        if (size > 0)
        {
            content.resize(static_cast<std::size_t>(size));
            in.seekg(0, std::ios::beg);
            in.read(&content[0], content.size());
        }

        callback(p, filename, content);
    }

    return true;
}

/**
 * @brief Read an entire file into a string (binary).
 * @param p File path.
 * @return File content, or empty string on failure.
 */
std::string read_file(const fs::path& p) noexcept
{
    std::error_code ec;
    UNUSED(ec);

    std::ifstream file(p, std::ios::binary);
    if (!file)
    {
        return {};
    }

    std::string data;

    file.seekg(0, std::ios::end);
    std::streampos size = file.tellg();

    if (size > 0)
    {
        data.resize(static_cast<std::size_t>(size));
        file.seekg(0, std::ios::beg);
        file.read(&data[0], data.size());
    }

    return data;
}

/**
 * @brief Compute MD5 of a file content.
 * @param p File path.
 * @return MD5 hex string of the file content, empty if file can't be read.
 */
std::string MD5_file(const fs::path& p) noexcept
{
    return Cache::MD5(read_file(p));
}


std::string MD5_file_bin(const fs::path& p) noexcept
{
    return Cache::MD5_bin(read_file(p));
}




//    ███╗   ███╗██████╗ ███████╗
//    ████╗ ████║██╔══██╗██╔════╝
//    ██╔████╔██║██║  ██║███████╗
//    ██║╚██╔╝██║██║  ██║╚════██║
//    ██║ ╚═╝ ██║██████╔╝███████║
//    ╚═╝     ╚═╝╚═════╝ ╚══════╝
//                               

namespace
{
    /**
     * @brief Internal MD5 state.
     */
    struct MD5Context
    {
        std::uint32_t h[4];
        std::uint64_t total_bytes;
        std::uint8_t  buffer[64];
        std::size_t   buffer_len;
    };

    /**
     * @brief Left rotate a 32-bit integer.
     */
    inline std::uint32_t leftrotate(std::uint32_t x, std::uint32_t c) noexcept
    {
        return (x << c) | (x >> (32U - c));
    }

    /**
     * @brief Initialize MD5 context.
     */
    void md5_init(MD5Context& ctx) noexcept
    {
        ctx.h[0]        = 0x67452301U;
        ctx.h[1]        = 0xefcdab89U;
        ctx.h[2]        = 0x98badcfeU;
        ctx.h[3]        = 0x10325476U;
        ctx.total_bytes = 0U;
        ctx.buffer_len  = 0U;
    }

    /**
     * @brief MD5 block transform (64 bytes).
     */
    void md5_transform(MD5Context& ctx, const std::uint8_t block[64]) noexcept
    {
        static const std::uint32_t K[64] =
        {
            0xd76aa478U, 0xe8c7b756U, 0x242070dbU, 0xc1bdceeeU,
            0xf57c0fafU, 0x4787c62aU, 0xa8304613U, 0xfd469501U,
            0x698098d8U, 0x8b44f7afU, 0xffff5bb1U, 0x895cd7beU,
            0x6b901122U, 0xfd987193U, 0xa679438eU, 0x49b40821U,
            0xf61e2562U, 0xc040b340U, 0x265e5a51U, 0xe9b6c7aaU,
            0xd62f105dU, 0x02441453U, 0xd8a1e681U, 0xe7d3fbc8U,
            0x21e1cde6U, 0xc33707d6U, 0xf4d50d87U, 0x455a14edU,
            0xa9e3e905U, 0xfcefa3f8U, 0x676f02d9U, 0x8d2a4c8aU,
            0xfffa3942U, 0x8771f681U, 0x6d9d6122U, 0xfde5380cU,
            0xa4beea44U, 0x4bdecfa9U, 0xf6bb4b60U, 0xbebfbc70U,
            0x289b7ec6U, 0xeaa127faU, 0xd4ef3085U, 0x04881d05U,
            0xd9d4d039U, 0xe6db99e5U, 0x1fa27cf8U, 0xc4ac5665U,
            0xf4292244U, 0x432aff97U, 0xab9423a7U, 0xfc93a039U,
            0x655b59c3U, 0x8f0ccc92U, 0xffeff47dU, 0x85845dd1U,
            0x6fa87e4fU, 0xfe2ce6e0U, 0xa3014314U, 0x4e0811a1U,
            0xf7537e82U, 0xbd3af235U, 0x2ad7d2bbU, 0xeb86d391U
        };

        static const std::uint32_t S[64] =
        {
            7U, 12U, 17U, 22U,  7U, 12U, 17U, 22U,  7U, 12U, 17U, 22U,  7U, 12U, 17U, 22U,
            5U,  9U, 14U, 20U,  5U,  9U, 14U, 20U,  5U,  9U, 14U, 20U,  5U,  9U, 14U, 20U,
            4U, 11U, 16U, 23U,  4U, 11U, 16U, 23U,  4U, 11U, 16U, 23U,  4U, 11U, 16U, 23U,
            6U, 10U, 15U, 21U,  6U, 10U, 15U, 21U,  6U, 10U, 15U, 21U,  6U, 10U, 15U, 21U
        };

        std::uint32_t w[16];

        // DECODE 64-BYTE BLOCK INTO 16 WORDS (LITTLE ENDIAN)
        for (int i = 0; i < 16; i++)
        {
            int j = i * 4;
            w[i]  = static_cast<std::uint32_t>(block[j]) |
                    (static_cast<std::uint32_t>(block[j + 1]) << 8U) |
                    (static_cast<std::uint32_t>(block[j + 2]) << 16U) |
                    (static_cast<std::uint32_t>(block[j + 3]) << 24U);
        }

        std::uint32_t a = ctx.h[0];
        std::uint32_t b = ctx.h[1];
        std::uint32_t c = ctx.h[2];
        std::uint32_t d = ctx.h[3];

        // MAIN MD5 ROUND
        for (std::uint32_t i = 0; i < 64U; i++)
        {
            std::uint32_t f;
            std::uint32_t g;

            if (i < 16U)
            {
                f = (b & c) | ((~b) & d);
                g = i;
            }
            else if (i < 32U)
            {
                f = (d & b) | ((~d) & c);
                g = (5U * i + 1U) & 0x0FU;
            }
            else if (i < 48U)
            {
                f = b ^ c ^ d;
                g = (3U * i + 5U) & 0x0FU;
            }
            else
            {
                f = c ^ (b | (~d));
                g = (7U * i) & 0x0FU;
            }

            std::uint32_t temp = d;
            d                  = c;
            c                  = b;
            std::uint32_t sum  = a + f + K[i] + w[g];
            b                  = b + leftrotate(sum, S[i]);
            a                  = temp;
        }

        // ADD THIS CHUNK'S HASH TO RESULT SO FAR
        ctx.h[0] += a;
        ctx.h[1] += b;
        ctx.h[2] += c;
        ctx.h[3] += d;
    }

    /**
     * @brief Update MD5 context with data bytes.
     */
    void md5_update(MD5Context& ctx, const std::uint8_t* data, std::size_t len) noexcept
    {
        ctx.total_bytes += static_cast<std::uint64_t>(len);

        std::size_t offset = 0U;

        // IF BUFFER HAS DATA, FILL UP TO 64 AND TRANSFORM
        if (ctx.buffer_len > 0U)
        {
            std::size_t to_copy = 64U - ctx.buffer_len;

            if (to_copy > len)
            {
                to_copy = len;
            }

            std::memcpy(ctx.buffer + ctx.buffer_len, data, to_copy);
            ctx.buffer_len += to_copy;
            offset         += to_copy;

            if (ctx.buffer_len == 64U)
            {
                md5_transform(ctx, ctx.buffer);
                ctx.buffer_len = 0U;
            }
        }

        // PROCESS FULL 64-BYTE BLOCKS
        while (offset + 64U <= len)
        {
            md5_transform(ctx, data + offset);
            offset += 64U;
        }

        // STORE REMAINDER
        if (offset < len)
        {
            std::size_t remain = len - offset;
            std::memcpy(ctx.buffer, data + offset, remain);
            ctx.buffer_len = remain;
        }
    }

    /**
     * @brief Finalize MD5 and write 16-byte digest.
     */
    void md5_final(MD5Context& ctx, std::uint8_t out[16]) noexcept
    {
        std::uint8_t padding[64] = { 0x80U };

        // PAD TO 56 BYTES MOD 64
        std::size_t pad_len;

        if (ctx.buffer_len < 56U)
        {
            pad_len = 56U - ctx.buffer_len;
        }
        else
        {
            pad_len = 64U - ctx.buffer_len + 56U;
        }

        md5_update(ctx, padding, pad_len);

        // APPEND LENGTH IN BITS (LITTLE ENDIAN)
        std::uint64_t total_bits = ctx.total_bytes * 8U;
        std::uint8_t  length_bytes[8];

        for (int i = 0; i < 8; i++)
        {
            length_bytes[i] = static_cast<std::uint8_t>((total_bits >> (8U * i)) & 0xFFU);
        }

        md5_update(ctx, length_bytes, 8U);

        // OUTPUT DIGEST
        for (int i = 0; i < 4; i++)
        {
            std::uint32_t word = ctx.h[i];

            out[i * 4 + 0] = static_cast<std::uint8_t>(word & 0xFFU);
            out[i * 4 + 1] = static_cast<std::uint8_t>((word >> 8U) & 0xFFU);
            out[i * 4 + 2] = static_cast<std::uint8_t>((word >> 16U) & 0xFFU);
            out[i * 4 + 3] = static_cast<std::uint8_t>((word >> 24U) & 0xFFU);
        }
    }
}



/**
 * @brief Compute MD5 for a string buffer.
 * @param data Input bytes.
 * @return Lowercase hex string (32 chars).
 */
std::string Cache::MD5(const std::string& data) noexcept
{
    MD5Context ctx;

    // INIT STATE
    md5_init(ctx);

    // UPDATE WITH INPUT
    md5_update(ctx, reinterpret_cast<const std::uint8_t*>(data.data()), data.size());

    // FINALIZE DIGEST
    std::uint8_t digest[16];
    md5_final(ctx, digest);

    // FORMAT AS LOWERCASE HEX
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::nouppercase;

    for (int i = 0; i < 16; i++)
    {
        oss << std::setw(2) << static_cast<int>(digest[i]);
    }

    return oss.str();
}


/**
 * @brief Compute MD5 for a string buffer.
 * @param data Input bytes.
 * @return Binary MD5 digest (16 raw bytes inside std::string).
 */
std::string Cache::MD5_bin(const std::string& data) noexcept
{
    MD5Context ctx;

    md5_init(ctx);
    md5_update(ctx,
               reinterpret_cast<const std::uint8_t*>(data.data()),
               data.size());

    std::uint8_t digest[16];
    md5_final(ctx, digest);

    // ritorna i 16 byte raw dentro una std::string
    return std::string(reinterpret_cast<const char*>(digest), 16);
}



//     ██████╗ █████╗  ██████╗██╗  ██╗███████╗
//    ██╔════╝██╔══██╗██╔════╝██║  ██║██╔════╝
//    ██║     ███████║██║     ███████║█████╗  
//    ██║     ██╔══██║██║     ██╔══██║██╔══╝  
//    ╚██████╗██║  ██║╚██████╗██║  ██║███████╗
//     ╚═════╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝╚══════╝
//                                            

/**
 * @brief Construct cache manager with default cache paths.
 */
Manager::Manager()
    :
    _cache_folder(".arcb"),
    _script_path(_P(_cache_folder) / _P("script")),
    _binary(_P(_cache_folder)),
    _store_idx(0),
    _cached_profile("")
{
    if (!dir_exists(_cache_folder))
    {
        create_dir(_cache_folder);
    }
}

void Manager::Freeze() noexcept
{
    uint64_t pos = CacheType::CT__PROFILE;

    _mnt_binary.reopen(_binary.string(), true);
    _mnt_binary.write_exact(pos, _cached_profile.data(), 16);

    pos = CacheType::CT__FILES;

    for(const auto& [key, value] : _cached_files)
    {
        _mnt_binary.write_exact(pos, key.data()         , 16);
        pos += 16;
        _mnt_binary.write_exact(pos, value.second.data(), 16);
        pos += 16;
    } 
 
    _mnt_binary.close();
}


void Manager::Store(const std::string& key) noexcept
{
    if (_store_idx == 0)
    {
        _mnt_binary.reopen(_binary.string(), true);
        _mnt_binary.write_exact(_store_idx, _cached_profile.data(), 16);
        _store_idx += 16;
    }

    const std::string md5_file = MD5_bin(key);

    _mnt_binary.write_exact(_store_idx, md5_file.data() , 16);
    _store_idx += 16;
    _mnt_binary.write_exact(_store_idx, _cached_files[md5_file].second.data(), 16);
    _store_idx += 16;
}


/**
 * @brief Erase the entire cache folder.
 */
void Manager::EraseCache() noexcept
{
    // REMOVE WHOLE CACHE TREE
    if (dir_exists(_cache_folder))
    {
        remove_dir_recursive(_cache_folder);
    }
}

/**
 * @brief Clear runtime cache content but keep cache root.
 *
 * This clears:
 * - generated scripts
 * - input hashes
 * - in-memory cached inputs
 *
 * Then it reloads the cache state from disk.
 */
void Manager::ClearCache(const std::vector<std::string>& keys) noexcept
{
    for (const auto& key : keys)
    {
        const std::string md5_file = MD5_bin(key);
        const auto it              = _cached_files.find(md5_file);

        if (it != _cached_files.end())
        {
            _cached_files.erase(it);
        }
    }
}

/**
 * @brief Ensure cache layout exists and load cached input hashes and profile marker.
 */
void Manager::LoadCache(const std::string& profile) noexcept
{
    // ENSURE SCRIPT PATH EXISTS
    if (!dir_exists(_script_path))
    {
        create_dir(_script_path);
    }

    _binary /= MD5(profile);

    if (!_mnt_binary.open(_binary.string()))
    {
        return;
    }

    if (!_mnt_binary.read_block16(CacheType::CT__PROFILE, _cached_profile))
    {
        _cached_profile = MD5_bin(_cached_profile);
        return;
    }

    // LOAD FILE RECORDS
    for (uint64_t off = CacheType::CT__FILES; off < _mnt_binary.file_size(); off += FILE_REC_SIZE)
    {
        char path_md5[MD5_RAW_SIZE];
        char content_md5[MD5_RAW_SIZE];

        if (!_mnt_binary.read_exact(off, path_md5, MD5_RAW_SIZE) || !_mnt_binary.read_exact(off + 16, content_md5, MD5_RAW_SIZE))
        {
            break;
        }

        const std::string k = std::string(path_md5,    MD5_RAW_SIZE);
        const std::string v = std::string(content_md5, MD5_RAW_SIZE);

        _cached_files.upsert(k, v, false);
    }

    return;
}
 

/**
 * @brief Check if a file changed since last cache snapshot.
 *
 * The file path string is hashed to form the key filename inside input cache.
 * The file content MD5 is stored as cache value for that key.
 *
 * @param path File path (string).
 * @return True if the file is new or changed, false otherwise.
 */
bool Manager::HasFileChanged(const std::string& path) noexcept
{
    const std::string md5_file    = MD5_bin(path);
    const std::string md5_content = MD5_file_bin(path);

    // IF NEW OR DIFFERENT, UPDATE CACHE
    return _cached_files.upsert(md5_file, md5_content, true); 
}



/**
 * @brief Write a script file for an instruction, using a stable name derived from job name and index.
 *
 * If the target script already exists, its content is compared via MD5 and rewritten only if different.
 *
 * @param jobname Job identifier.
 * @param idx Instruction index.
 * @param content Script content to write.
 * @param ext Optional extension (e.g. ".bat").
 * @return Path to generated script file.
 */
fs::path Manager::WriteScript(const std::string& jobname,
                              const std::size_t idx,
                              const std::string& content,
                              const std::string& ext) noexcept
{
    fs::path    script_path;
    std::string md5_filename = MD5(jobname);

    // BUILD SCRIPT PATH
    if (ext.empty())
    {
        script_path = _script_path / (md5_filename + std::to_string(idx));
    }
    else
    {
        script_path = _script_path / (md5_filename + std::to_string(idx) + ext);
    }

    // UPDATE FILE ONLY IF CONTENT CHANGED
    if (file_exists(script_path))
    {
        std::string md5_oldcontent = MD5_file(script_path);
        std::string md5_newcontent = MD5(content);

        if (md5_oldcontent.compare(md5_newcontent) != 0)
        {
            create_file(script_path, content);
        }
    }
    else
    {
        create_file(script_path, content);
    }

    return script_path;
}
