# Changelog

All relevant changes to this project will be documented in this file.

## [0.7.0] - 2025-03-09
Major Release **Lushy Lion** (v 0.7.0)  

### Changes
- Renamed the tool from **arcana** to **arcb**
- Variable access grammar from {arc:[algo:]VARNAME} to arcb::VARNAME[.method()] 
- Attribute **interpreter** to **engine** 
- List method now follows the grammar **arcb::VARNAME.list([l[, r]])**  
  The optional parameters control the selected range:

            list()       → expands over the entire variable
            list(r)      → expands from index 0 to r
            list(l, r)   → expands from index l to r
- Inline method now follows the grammar **arcb::VARNAME.inline()**

### Added
- Attribute **death**
- New DSL for engine attribute that follows: **@engine \<type\> \[ext\] \<path\>** 
- Variable method **size** that returns the number of elements of the variable as a string
- Variable mathod **empty** that returns 1 if the variable is empty, otherwise 0

### Fixed Bugs
- Minor bugs

## [0.6.0] - 2025-02-24
Major Release **Lushy Lion** (v 0.6.0)  

### Removed
- Attribute **flushcache**
- Removed task params

### Changes
- Cache module logic
- Improved job graph generation & visit
- Moved task instructions expansion logic

### Added
- Attribute **cache**, with commands: **track**, **untrack** and **store**

## [0.5.0] - 2025-02-11
### Changes
- Improved the Glob engine performance
- Minor performance improves

### Added
- Variable sum (+= semantic)
- Assert with recovery callbacks list
- **@glob** attribute, from now a glob pattern must be specified as glob
- **\_\_release__** builtin symbol
- Ability to search for multiple glob patterns and optionally map them into one

### Fixed Bugs
- Alingned variable declaration grammar regex to variable expansion regex 


## [0.4.3] - 2025-12-21
### Removed
- Module **Debug**

### Added
- CLI optons: **--value \<VALUE>**, **--pubs** and **--profiles** 

### Fixed Bugs
- **@main** attribute logic  


## [0.4.2] - 2025-12-20
### Changes
- Refactored Expader logic
- Added keyword **in** for **assert** statement  
- Added filesystem seach logic for **assert** statement via  
  **{fs:xxx}** expasion type  


## [0.4.1] - 2025-12-19

### Changes
- Improved windows support
- Reworked Makefile in order to include windows compilation 
- Messages error printing

### Added
- Variable expasion support for attribute @interpreter
- new **assert** statement:
```arcb
assert "{arc:__os__}" eq "linux" -> "reason message";
```

### Fixed Bugs
- Fixed script generation and execution


## [0.4.0] - 2025-12-18

### Added
- Custom Glob engine [BETA]
- Windows compatibility (minGW) [BETA]
- New attribute **@ifos**
- New allocator for builtin symbols

## [0.3.1] - 2025-12-13

### Added
- New statement **map** (same as @map attribute), with syntax:
```arcb
map <SOURCE> -> <TARGET>;
```

## [0.3.0] - 2025-12-12

### Added
- New builtin symbols:
  - \_\_main__
  - \_\_root__
  - \_\_threads__
  - \_\_max_threads__
  - \_\_os__
  - \_\_arch__ 
- Updated script generation logic in cache module.
- CRLF normalization in Lexer module.
- New CLI option: **--silent**

### Fixed Bugs
- Fixed 'inline' logic expasion of glob variables.
- Fixed profile switching not invalidating cache.

---

## [0.2.0] - 2025-12-07

### Added
- New cache invalidation logic for build graph.
- New attribute **exclude**

### Fixed Bugs
- Fixed minor bugs.

---

## [0.1.0] - 2025-12-05

### Added
- First public version of Arcana.
- Parser, Semantic analysis and Job generation.
- Job executor.
- Basic cache logic.
