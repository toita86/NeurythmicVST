Weekly outcome:
Establish the development environment and inspect the real code.

Why it matters:
The aim is to remove the largest risks aka `cpgLib` builds reproducibly

Tasks:
- clone cpglib.
- create a project notebook or repository wiki.
- record compiler, cmake and operating-system versions.
- attempt to configure and build cpglib unchanged.
- identify the important source files and classes.
- read the original system description while tracing corresponding code.
- create an “unknowns and risks” list.

Proof:
- a reproducible build command
- a documented build-failure report containing the exact errors and attempted fixes.

Blockers:
None

What I will not do:
- design the final UI;
- rewrite library. 

---

Completed:
- clone cpglib.
- create a project notebook or repository wiki.
- record compiler, cmake and operating-system versions.
- attempt to configure and build cpglib unchanged.
- identify the important source files and classes.
- read the original system description while tracing corresponding code.
- create an “unknowns and risks” list.   

Unexpected learning:
The linking and compiling went smooth even when trying to bump the C++ version.

Remaining blocker:

Next smallest step:
Testing the code with a small snippet that test the capability of the library 
