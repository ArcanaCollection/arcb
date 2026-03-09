```
 █████╗ ██████╗  ██████╗ █████╗ ███╗   ██╗ █████╗ 
██╔══██╗██╔══██╗██╔════╝██╔══██╗████╗  ██║██╔══██╗
███████║██████╔╝██║     ███████║██╔██╗ ██║███████║
██╔══██║██╔══██╗██║     ██╔══██║██║╚██╗██║██╔══██║
██║  ██║██║  ██║╚██████╗██║  ██║██║ ╚████║██║  ██║
╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝
```                             
---
[![forthebadge](https://img.shields.io/badge/Based-gray?style=flat&logo=c%2B%2B&labelColor=546CAF)]()  

[![forthebadge](https://img.shields.io/badge/GNU%2FLinux-Support-gray?labelColor=FFC602&logo=linux&logoColor=black&link=https://www.getgnulinux.org/en/)](https://www.getgnulinux.org/en/)  
[![forthebadge](https://img.shields.io/badge/Windows-Support%20[BETA]-gray?labelColor=0078D6&logo=windows&logoColor=black)]()  

[![forthebadge](https://img.shields.io/badge/Version-0.4.3-green)](https://gitlab.com/MastroGibbs/arcb)
---

# arcb — an alternative to traditional build systems

arcb is built around a simple idea:  
**your build logic should be clear, predictable, and entirely under your control.**

Where many build systems accumulate implicit rules, ad-hoc behavior, and oversized DSLs, arcb deliberately goes the opposite way.  
It gives you a compact language that describes your build steps without noise and without pretending to be a programming language.

arcb is not trying to replace huge ecosystems.  
It aims to be a tool you can **read**, **understand**, and **maintain** effortlessly.

---

## ✨ Why arcb?

### **A minimal language you can actually read**
arcb’s DSL is intentionally small:
- variables  
- tasks  
- attributes  

That’s it.  
No hidden logic, no flow control, no cryptic operators.

---

### **Tasks**
A task is a clear, explicit unit of work.  
You write the commands; arcb handles orchestration:

```arcb
task Compile(SOURCES) {
    gcc -c {arcb::list:SOURCES}
}
```
Inputs tell the engine what the task depends on, enabling precise caching.


### **Variables**
Globbing that behaves naturally
Patterns such as:

```arcb
src/**.c
```
expand automatically at runtime.
If a variable isn’t a glob, it is passed through unchanged.


### **Execution**
Profiles without the usual complexity
Switch build profiles cleanly:

```bash
arcb build -p Debug
```
or
```bash
arcb build -p Release
```
Profiles modify behavior without injecting conditional logic into the DSL.  
Attributes that extend, not complicate.  
Attributes add metadata to tasks: ordering, interpreters, multithreading, cache behavior, and more.  
They enhance your workflow without adding hidden semantics.  
Predictable caching.  

arcb automatically checks whether a task’s inputs have changed:  
**unchanged → task skipped**  
**modified → task executed**  


Use your preferred interpreter:  
Task bodies run in the system’s interpreter (bash by default).

arcb does not embed its own programming language.  
If your shell can run it, arcb can orchestrate it.

## 🧠 Philosophy
**Transparency**: no invisible rules.  
**Minimalism**: a tool stays maintainable by staying small.  
**User control**: you describe the build; arcb executes it.  

## 🚀 Getting Started
Create an arcfile at your project root, by the generator.

```bash
arcb --generate
```

Then customize it and run:

```bash
arcb
```
Run a specific task:

```bash
arcb <task>
```



## 🧩 Who is arcb for?
arcb fits well for:

- developers who prefer explicit build logic over implicit rules
- small and medium projects that don’t need heavyweight ecosystems
- teams that want build scripts everyone can read
- users frustrated by Make, yet unwilling to adopt massive systems like CMake
- anyone who wants structured task orchestration without sacrificing shell freedom


If you want a builder that does its job without getting in your way, arcb fits perfectly.


## 📄 License
See the repository for license details.

Contributions, issues, and feature ideas are welcome.


## 🛠️ Developed and mainteined by
[MastroGibbs (GitLab)](https://gitlab.com/MastroGibbs)  
[MastroGibbs (GitHub)](https://github.com/Mastro-Gibbs)
