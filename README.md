# Coreutils++

Reimplementation of some of the GNU Coreutils in C++, useful to be used in Windows' cmd.exe.

## cat

```
$ cat README.md
```

## env

```
$ env -C /tmp ls
```

## ls

```
$ ls
.cache               .clang-format        .git                 .gitignore           .gitmodules
.vs                  azure-pipelines.yml  build                CMakeLists.txt       CMakeSettings.json          
LICENSE              out                  output_test.py       README.md            src                         
subprojects          windows
```

## rm

Shows a progress indication and warns when you're trying to delete your `HOME` directory or your current working
directory.

For an even safer way to remove files, see [rip](https://github.com/nivekuil/rip).

## touch

```
$ touch some_new_file
```
