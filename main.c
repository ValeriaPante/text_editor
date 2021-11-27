#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define INITIAL_CAPACITY 80000
#define RESIZE_INCREMENT 10000
#define MAX_LINE_SIZE 1024

typedef struct line_t {
    char* text;
    struct line_t* next;
} Line;

typedef struct command_t {
    char type; // c = change, d = delete, n = null
    int fromLine;
    int toLine;
    int newSize;
    Line* newTail;
    Line* beforeLine;
    Line* afterLine;
    Line* headOldSequence;
    Line* headNewSequence;
} Command;

typedef struct stack_t{
    Command** commands;
    long size;
    long capacity;
    long redoOp;
    long undoOp;
    long stackPointer;
    long lastValid;
} Stack;

typedef struct {
    Line* head;
    Line* tail;
    int linesCounter;
    Stack commands;
    bool undoStrike;
} Text;

Line* getLineBefore(int lineIndex, Text* text) {
    // Caso 1: linea prima della head
    Line *beforeLine = NULL;
    if (lineIndex > text->linesCounter + 1) return NULL;
    // Caso 2: indice fuori range
    if (lineIndex > text->linesCounter && text->linesCounter > 0) return text->tail;
    // Caso3: linea prima di un nodo al centro o prima della tail
    if (lineIndex > 1) {
        beforeLine = text->head;
        for (int i = 2; i < lineIndex; i++) {
            beforeLine = beforeLine->next;
        }
    }
    return beforeLine;
}

Line* getLine(int lineIndex, int fromLine, Line* init, Text* text) {
    if( lineIndex == text->linesCounter ) return text->tail;
    if( lineIndex > text->linesCounter ) return NULL;
    Line *line = NULL;
    if( init == NULL ) line = text->head;
    else line = init->next;

    for( int i = fromLine+1; i < lineIndex+1 && line->next != NULL; i++) {
        line = line->next;
    }

    return line;
}

Line* createNewLine(char *newText) {
    Line* newLine = malloc(sizeof(Line));
    if( newLine == NULL ) return NULL;
    newLine->text = malloc(sizeof(char) * (strlen(newText) + 1));
    if( newLine->text == NULL ) return NULL;
    strcpy(newLine->text, newText);
    newLine->next = NULL;
    return newLine;
}

char* getInput(char* buf) {
    fgets(buf, MAX_LINE_SIZE, stdin);
    return buf;
}
Line* getNewLine(char* buf) {
    return createNewLine(getInput(buf));
}

void resetCommand(Command* c, int from, int to, char type) {
    c->type = type;
    c->fromLine = from;
    c->toLine = to;
    c->newSize = 0;
    c->afterLine = NULL;
    c->beforeLine = NULL;
    c->newTail = 0;
    c->headNewSequence = NULL;
    c->headOldSequence = NULL;
}
Command* createCommand(int from, int to, char type) {
    Command* c = malloc(sizeof(Command));
    if( c == NULL ) exit(EXIT_FAILURE);
    resetCommand(c, from, to, type);
    return c;
}
void copyCommandAndSetInvalid(Command* src, Command* dst, Stack* stack) {
    if( src == NULL ) {
        resetCommand(dst, 'n', 0, 0);
        return;
    }
    dst->type = 'n';
    dst->fromLine = src->fromLine;
    dst->toLine = src->toLine;
    dst->newSize = src->newSize;
    dst->afterLine = src->afterLine;
    dst->beforeLine = src->beforeLine;
    dst->newTail = src->newTail;
    dst->headNewSequence = src->headNewSequence;
    dst->headOldSequence = src->headOldSequence;

}

void initStack(Stack* stack) {
    stack->commands = calloc(sizeof(Command*), INITIAL_CAPACITY);
    if(stack->commands == NULL) return;
    stack->capacity = INITIAL_CAPACITY;
    stack->size = 0;
    stack->stackPointer = 0;
    stack->lastValid = 0;
    stack->redoOp = 0;
    stack->undoOp = 0;
}

void clearStack(Stack* stack) {
    stack->size = stack->stackPointer;
    stack->undoOp = 0;
    stack->redoOp = 0;
}

void resize(Stack *stack) {
    if( stack->size + 1 > stack->capacity ) {
        long oldCapacity = stack->capacity;
        stack->capacity += RESIZE_INCREMENT;
        stack->commands = realloc(stack->commands, stack->capacity * sizeof(Command*));
        if( stack->commands == NULL ) exit(1);
        memset(stack->commands +  oldCapacity, 0, RESIZE_INCREMENT * sizeof(Command*));
    }
}

Command* pushNewCommand(Stack* stack, int from, int to, char type) {
    stack->size++;
    resize(stack);
    stack->stackPointer++;

    Command* newCommand = stack->commands[stack->stackPointer];
    if( newCommand != NULL ) {
        resetCommand(newCommand, from, to, type);
    } else {
        newCommand = createCommand(from, to, type);
        stack->commands[stack->stackPointer] = newCommand;
    }

    return newCommand;
}

Command* top(Stack* stack) {
    if( stack->stackPointer == 0 ) return NULL;
    return stack->commands[stack->stackPointer];
}

void popRedo(Stack* stack) {
    stack->stackPointer++;
}

void popUndo(Stack* stack) {
    stack->stackPointer--;
}

void updateLastValid(Stack* stack) {
    for(int i = stack->stackPointer; i > 0; i--) {
        if(stack->commands[i]->type != 'n') {
            stack->lastValid = i;
            return;
        }
    }
    stack->lastValid = 0;
}

void initText(Text* t) {
    t->linesCounter = 0;
    t->head = NULL;
    t->tail = t->head;
    t->undoStrike = false;
    initStack(&t->commands);
}

void printText(const int fromLine, const int toLine, Text* text) {
    Line* line = text->head;
    int startingLine = fromLine;
    while( startingLine < 1 ) {
        printf(".\n");
        ++startingLine;
    }

    for(int i = 1; i <= toLine; i++) {
        if(i >= startingLine ) {
            if( line == NULL ) printf(".\n");
            else printf("%s", line->text);
        }
        if( line != NULL ) line = line->next;
    }
}

void setCommandMetaData(Command* command, Text* text) {
    char buf[MAX_LINE_SIZE];
    command->headNewSequence = getNewLine(buf);

    if(command->beforeLine == NULL) {
        if( text->linesCounter == 0 ) command->headOldSequence = NULL;
        else command->headOldSequence = text->head;
    } else command->headOldSequence = command->beforeLine->next;

    if(command->headOldSequence != NULL ) command->afterLine = command->headOldSequence->next;
}

void processLines(Command* command, Text* text) {
    char buf[MAX_LINE_SIZE];
    setCommandMetaData(command, text);
    Line* line = command->headNewSequence;
    Line* sequenceTail;
    while( true ) {
        getInput(buf);
        if( buf[0] == '.' && buf[1] == '\n' ) {
            sequenceTail = line;
            break;
        }
        line->next = createNewLine(buf);
        line = line->next;
        if(command->afterLine != NULL) command->afterLine = command->afterLine->next;
    }

    if( command->toLine >= text->linesCounter ) {
        text->tail = sequenceTail;
        if( command->toLine > text->linesCounter ) text->linesCounter += command->toLine - text->linesCounter;
    } else sequenceTail->next = command->afterLine;
    if( command->beforeLine == NULL ) text->head = command->headNewSequence;
    else command->beforeLine->next = command->headNewSequence;
}

bool checkDeleteValidity(int* fromLine, int* toLine, Text* text) {
    Stack* stack = &text->commands;
    if( *toLine > text->linesCounter ) {
        *toLine = text->linesCounter;
        top(stack)->toLine = *toLine;
    }

    if( *fromLine < 1 ) {
        *fromLine = 1;
        top(stack)->fromLine = *fromLine;
    }

    if( *fromLine > text->linesCounter || *toLine < 1 ) {
        Command* lastCommand = top(stack);
        copyCommandAndSetInvalid(stack->commands[stack->lastValid], lastCommand, stack);
        return false;
    }

    stack->lastValid = stack->stackPointer;
    return true;
}

void delete(Line* beforeLine, Line* afterLine, int deleted, Text* text) {

    if( afterLine == NULL ) text->tail = beforeLine; // cancello la coda
    if( beforeLine == NULL ) text->head = afterLine; // cancello la testa
    else beforeLine->next = afterLine; // cancello elementi al centro

    text->linesCounter -= deleted;
}

void deleteFirstTime(int fromLine, int toLine, Text* text) {
    if( !checkDeleteValidity(&fromLine, &toLine, text) ) return;
    Command* command = top(&text->commands);
    Line* beforeLine = getLineBefore(fromLine, text);
    command->beforeLine = beforeLine;
    if( beforeLine == NULL ) command->headOldSequence = text->head;
    else command->headOldSequence = beforeLine->next;

    command->afterLine = getLine(toLine + 1, fromLine, beforeLine, text);

    int deleted = toLine - fromLine + 1;
    delete(beforeLine, command->afterLine, deleted, text);
    command->newSize = text->linesCounter;
    command->newTail = text->tail;
}

void incrementUndo(char* buf, Text* text) {
    int operations = atoi(buf);
    text->undoStrike = true;

    int maxUndos = text->commands.stackPointer + text->commands.redoOp;
    if( operations > maxUndos ) operations = maxUndos;
    if( operations + text->commands.undoOp  > maxUndos  )  text->commands.undoOp = maxUndos;
    else text->commands.undoOp += operations;

}

void incrementRedo(char* buf, Text* text) {
    int operations = atoi(buf);
    text->undoStrike = true;
    int maxRedos = text->commands.undoOp + text->commands.size - text->commands.stackPointer;
    if( operations > maxRedos ) operations = maxRedos;
    if( operations + text->commands.redoOp > maxRedos ) text->commands.redoOp = maxRedos;
    else text->commands.redoOp += operations;
}

bool tryRestoreVersion(int textSize, Text* text, Line* tail) {
    bool restorable = false;
    if( textSize == 0 ) {
        text->head = NULL;
        text->tail = NULL;
        restorable = true;
    }
    else if( textSize == 1 ) {
        text->tail = tail;
        text->head = text->tail;
        restorable = true;
    }
    if( restorable ) text->linesCounter = textSize;
    return restorable;
}

void undo(int undoOp, Text *text) {
    Stack* stack = &text->commands;
    long oldestCommandIndex = stack->stackPointer - undoOp + 1;
    if( stack->lastValid < oldestCommandIndex ) {
        stack->stackPointer -= undoOp;
        return;
    }
    if( oldestCommandIndex - 1 < 1 ) {
        tryRestoreVersion(0, text, NULL);
        stack->stackPointer -= undoOp;
        stack->lastValid = 0;
        return;
    }
    Command* oldInfo = stack->commands[oldestCommandIndex - 1];
    int oldTextSize = oldInfo->newSize;
    Line* oldTail = oldInfo->newTail;
    if( tryRestoreVersion(oldTextSize, text, oldTail) ) {
        stack->stackPointer -= undoOp;
        updateLastValid(stack);
        return;
    }
    while(undoOp > 0) {
        Command* command = top(stack);
        if( command->type != 'n' && command->fromLine <= oldTextSize ) {
            if( command->beforeLine == NULL ) text->head = command->headOldSequence;
            else command->beforeLine->next = command->headOldSequence;
        }
        undoOp--;
        popUndo(stack);
    }
    updateLastValid(stack);
    text->tail = oldTail;
    text->linesCounter = oldTextSize;
    if( text->tail!= NULL ) text->tail->next = NULL;
}

void redo(int redoOp, Text *text) {
    Stack* stack = &text->commands;
    Command* newestCommand = stack->commands[stack->stackPointer+redoOp];
    int newTextSize = newestCommand->newSize;
    if( tryRestoreVersion(newTextSize, text, newestCommand->newTail) ) {
        stack->stackPointer += redoOp;
        updateLastValid(stack);
        return;
    }
    while(redoOp > 0) {
        popRedo(stack);
        Command* command = top(stack);
        if( command->type != 'n' && command->fromLine <= newTextSize ) {
            Line* firstLineSequence;
            if( command->type == 'd') firstLineSequence = command->afterLine;
            else firstLineSequence = command->headNewSequence;

            if( command->beforeLine == NULL ) text->head = firstLineSequence;
            else command->beforeLine->next = firstLineSequence;
        }

        redoOp--;
    }
    updateLastValid(stack);
    text->tail = newestCommand->newTail;
    if( text->tail!= NULL ) text->tail->next = NULL;
    text->linesCounter = newTextSize;
}

void undoRedo(Text *text) {
    Stack* commands = &text->commands;
    int undoOp = commands->undoOp - commands->redoOp;
    if( undoOp > 0 ) undo(undoOp, text);
    else if( undoOp < 0 ) redo(abs(undoOp), text);

    commands->undoOp = 0;
    commands->redoOp = 0;
}
void checkUndo(Text* text, char commandType) {
    if( text->undoStrike ) {
        undoRedo(text);
        if( (commandType == 'c' || commandType == 'd') ) {
            clearStack(&text->commands);
            text->undoStrike = false;
        }
    }
}

bool isChangeDeletePrint(char* buf, char commandType, bool* changeSequence, Text* text) {
    int toLine = atoi( strchr( buf, ',' ) + 1 );
    int fromLine = atoi(buf);

    checkUndo(text, commandType);
    switch (commandType) {
        case 'c':
            pushNewCommand(&text->commands, fromLine, toLine,commandType);
            *changeSequence = true;
            top(&text->commands)->beforeLine = getLineBefore(fromLine, text);
            break;
        case 'd':
            pushNewCommand(&text->commands, fromLine, toLine,commandType);
            deleteFirstTime(fromLine, toLine, text);
            break;
        default:
            printText(fromLine, toLine, text);
    }
    return true;
}

int main() {
    Text text;
    initText(&text);
    bool changeSequence = false;
    char buf[30];

    while( true ) {

        if( changeSequence ) {
            Command* lastCommand = top(&text.commands);
            text.commands.lastValid = text.commands.stackPointer;
            processLines(lastCommand, &text);
            lastCommand->newSize = text.linesCounter;
            lastCommand->newTail = text.tail;
            changeSequence = false;
        } else {
            fgets(buf, sizeof(buf), stdin);
            char command = buf[ strlen(buf) - 2 ]; //conto dalla fine
            if( buf[0] == 'q' && buf[1] == '\n' ) return 0;
            if( command == 'u' ) incrementUndo(buf, &text);
            else if( command == 'r' ) incrementRedo(buf, &text);
            else isChangeDeletePrint(buf, command, &changeSequence, &text);
        }
    }
}