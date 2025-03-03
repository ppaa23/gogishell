[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/N96cjnwk)
# GoGiShell

A custom-built command-line interface that provides a user-friendly environment for interacting with the
operating system.

## Features

1. [Feature 1](../../issues/1)
   - Showing commands from history in-line
   - If UP or DOWN is pressed, the input line replaces with line from history (buttons allow to navigate through commands list).

2. [Feature 2](../../issues/2)
   - Ability to create custom abbreviations
   - If input is "setabbr \<value\> \<key\>", value and key are writing to a file, and in all following inputs <key> replaces with <value> before executing. <Value> might contain whitespaces.

3. [Feature 3](../../issues/3)
   - Autocomplete of commands according to its frequency of use
   - If TAB is pressed, current command completes to the most used command from history starting the same way (if TAB is pressed when input is empty, it just completes to the most used command).

4. [Feature 4](../../issues/4)
   - Ability to add labels (consisting of a description and optionally a color) to folders
   - If input is "ldir /path/to/the/folder -d description /[-c color/]", this directory will be provided with description (shown by entering directory) and color in prompts.

5. [Feature 5](../../issues/5)
   - Ability to redirect input and output
   - If input contains '>', '<' or '>>', it will be recognized as command to redirect input/output to the required source/destination.

6. [Feature 6](../../issues/6)
   - Ability to use pipelines
   - If input contains '|', output of the input before pipe will be redirected to the part after it.

## Dependencies

- GCC
- Make
- BASH

## Build Instructions

1. Clone the repository:
```bash
git clone https://github.com/ppaa23/gogishell.git
cd gogishell
```

2. Build the project:
```bash
make
```

3. Run tests:
```bash
make test
```
Be careful, testing deletes all your cache

## Usage Examples

```bash
make run
```
or
```bash
cd build
./GoGiShell
```
After launching pseudoshell you can use internal BASH command and GoGiShell commands as well.
Here is video capture showing basic functionality and features of GoGiShell: https://www.youtube.com/watch?v=DZAADn251h0
