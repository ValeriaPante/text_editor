# Text Editor

The project consists in implementing a simple text editor. The editor considers a document as a sequence of lines, of arbitrary size, numbered starting from one.
The editor interface is made up of text commands, terminated by a "newline" character. The commands can be followed by a portion of text, consisting of one or more lines, terminated by a '.' (period) which appears as the only character on the next line. The commands consist of a single letter, optionally preceded by one or two whole numbers.

The editor interface is freely inspired by that of the traditional editor *ed*.

In some commands, the integers that appear in them represent address specifiers. More precisely, an address specifier is a number n, expressed in decimal, which indicates the address of the n-th line; the first line of the text has address 1.

The supported commands are the following, with the convention that ind1, ind2 indicate two address specifiers such that ind1 ≤ ind2 and round brackets are introduced for ease of reading this text, but not included in the command:
- (ind1, ind2) c

  Change the text present on the lines between ind1 and ind2 (extremes included). The text following the command must be made up of a number of lines equal to ind2-ind1 + 1. ind1 must be either an address actually present in the text, or the first address after the last line in the text (or 1 if the text is still empty).
  
- (ind1, ind2) d

  Delete the lines between ind1 and ind2 (extremes included), moving up the lines following that of address ind2 (if there are any). Deleting a line that does not exist in the text has no effect.
  
- (ind1, ind2) p

  Print the lines between ind1 and ind2, inclusive. Where there is no line in the text in the position to be printed, a line is printed containing only the character '.' Followed by a "return".
  
- (number) u 

  Undo (undo) a number of commands (c or d) equal to that specified in round brackets (where number is an integer strictly greater than zero). A sequence of consecutive undo commands cancels a number of steps equal to the sum of the steps specified in each of them. If the number of commands to be canceled is higher than the number of executed commands, all steps are canceled. The execution of a text modification command (c, d) after an undo cancels the effects of the definitively canceled commands. Note that the commands that have no effect are also counted in the number of commands to be canceled (for example, the cancellation of a block of lines that do not exist).
  
- (number) r

  Cancels the undo effect for a number of commands equal to the number starting from the current version (redo function). Note that number must be an integer strictly greater than zero. We therefore have a sequence of commands of the type 10u 5r
It is to all intents and purposes equivalent to the 5u command only. Similarly, the sequence 12u 3r 2u 7r equivalent to the 4u command. In the event that the number of commands to be redoed is greater than those currently canceled, the maximum number of possible redos is performed.

- q

  Finish the execution of the editor
  
A line of text supplied to the editor can contain a maximum of 1024 characters.

Assume that only correct commands are given to the editor (it is therefore not necessary to verify their correctness). For example, a command c is never given in which ind1> ind2, or ind1 <1, or ind1 is not the address of a line in the text, nor is the address of the first line after the text. Similarly for the other commands. 
However, pay attention to the fact that, in some cases, the command is allowed, but it simply has no effect; for example, it is possible to give a command d in which ind1 is not the address of a line of text, but in this case the command has no effect on the text.

In this repository you can find the implemented editor and a set of tests to which it has been subjected.
