/* ECE 309 Project 1: a small, deterministic LLM agent harness. */
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HISTORY_LIMIT 5
#define INPUT_SIZE 256       /* 255 bytes of text plus the terminating null. */
#define RESPONSE_SIZE 512

/* One conversation turn includes both sides of the interaction. */
typedef struct {
    char user[INPUT_SIZE];
    char response[RESPONSE_SIZE];
} Turn;

/* Skip whitespace safely, including when char is signed on this system. */
static const char *skip_spaces(const char *text)
{
    while (isspace((unsigned char)*text)) {
        text++;
    }
    return text;
}

static void print_help(void)
{
    puts("Commands: hello, calc NUMBER OP NUMBER, history, recall, help, exit");
    puts("Calculator operators: + - * / (example: calc 12 * 3)");
    puts("Other text is echoed. History keeps the last 5 user/response pairs.");
}

/* Displaying history does not change it. Entries are oldest first. */
static void print_history(const Turn history[], int count)
{
    printf("History (%d/%d turns):\n", count, HISTORY_LIMIT);
    for (int i = 0; i < count; i++) {
        printf("%d. User: %s\n", i + 1, history[i].user);
        printf("   Assistant: %s\n", history[i].response);
    }
}

static void save_turn(Turn history[], int *count,
                      const char *input, const char *response)
{
    /* When full, discard the oldest complete pair by shifting left. */
    if (*count == HISTORY_LIMIT) {
        for (int i = 1; i < HISTORY_LIMIT; i++) {
            history[i - 1] = history[i];
        }
        (*count)--;
    }

    /* Clear the reused slot and copy both strings with explicit bounds. */
    history[*count] = (Turn){0};
    snprintf(history[*count].user, INPUT_SIZE, "%.*s", INPUT_SIZE - 1, input);
    snprintf(history[*count].response, RESPONSE_SIZE, "%s", response);
    (*count)++;
}

/* Match lowercase hello as a word, including in "please say hello!". */
static int contains_hello(const char *input)
{
    const char *word = input;
    while ((word = strstr(word, "hello")) != NULL) {
        if ((word == input || !isalpha((unsigned char)word[-1])) &&
            !isalpha((unsigned char)word[5])) {
            return 1;
        }
        word++;
    }
    return 0;
}

/* The mock model reads prior context but never changes the history itself. */
static void mock_model(const char *input, const Turn history[], int count,
                       char response[])
{
    if (strcmp(input, "recall") == 0) {
        if (count == 0) {
            snprintf(response, RESPONSE_SIZE, "No previous conversation turn.");
        } else {
            snprintf(response, RESPONSE_SIZE, "Previous input: %s",
                     history[count - 1].user);
        }
    } else if (contains_hello(input)) {
        snprintf(response, RESPONSE_SIZE, "Hello! I am a mock model.");
    } else {
        snprintf(response, RESPONSE_SIZE, "You said: %s", input);
    }
}

/* Arithmetic belongs in a tool: the mock model does not guess the answer. */
static void calculator(const char *expression, char response[])
{
    const char *cursor = skip_spaces(expression);
    char *end;
    double left, right, result;
    char operation;

    /* strtod lets us detect failed conversions and out-of-range numbers. */
    errno = 0;
    left = strtod(cursor, &end);
    if (cursor == end || errno == ERANGE || !isfinite(left)) {
        snprintf(response, RESPONSE_SIZE, "Calculator error: invalid first number.");
        return;
    }

    cursor = skip_spaces(end);
    operation = *cursor;
    if (operation != '+' && operation != '-' &&
        operation != '*' && operation != '/') {
        snprintf(response, RESPONSE_SIZE, "Calculator error: use +, -, *, or /.");
        return;
    }

    cursor = skip_spaces(cursor + 1);
    errno = 0;
    right = strtod(cursor, &end);
    if (cursor == end || errno == ERANGE || !isfinite(right)) {
        snprintf(response, RESPONSE_SIZE, "Calculator error: invalid second number.");
        return;
    }
    if (*skip_spaces(end) != '\0') {
        snprintf(response, RESPONSE_SIZE, "Calculator error: unexpected trailing text.");
        return;
    }
    if (operation == '/' && right == 0.0) {
        snprintf(response, RESPONSE_SIZE, "Calculator error: division by zero.");
        return;
    }

    switch (operation) {
        case '+': result = left + right; break;
        case '-': result = left - right; break;
        case '*': result = left * right; break;
        default:  result = left / right; break; /* Already validated as '/'. */
    }

    if (!isfinite(result)) {
        snprintf(response, RESPONSE_SIZE, "Calculator error: result out of range.");
    } else {
        snprintf(response, RESPONSE_SIZE, "Calculator result: %.10g", result);
    }
}

int main(void)
{
    /* Fixed automatic storage: no malloc/free or heap ownership is needed. */
    Turn history[HISTORY_LIMIT] = {0};
    int count = 0;
    /* Extra room accepts 255 text bytes plus either LF or CRLF and a null. */
    char input[INPUT_SIZE + 2];
    char response[RESPONSE_SIZE];

    puts("ECE 309 Mini Harness");
    puts("Type help for commands. Type exit to quit.");

    while (1) {
        size_t length;
        printf("You> ");
        fflush(stdout); /* Show the prompt before waiting for terminal input. */

        if (fgets(input, sizeof(input), stdin) == NULL) {
            if (ferror(stdin)) {
                perror("Input error");
                return EXIT_FAILURE;
            }
            break; /* End-of-file (Ctrl+D on Linux) is also a clean exit. */
        }

        length = strlen(input);
        if (length > 0 && input[length - 1] == '\n') {
            input[--length] = '\0';
        } else if (length == sizeof(input) - 1) {
            /* Drain an overlong line so its remainder is never a command. */
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {
                /* Discard the remainder of this one line. */
            }
            if (ferror(stdin)) {
                perror("Input error");
                return EXIT_FAILURE;
            }
        }
        if (length > 0 && input[length - 1] == '\r') {
            input[--length] = '\0';
        }
        if (length >= INPUT_SIZE) {
            puts("Input too long (maximum 255 bytes). Turn ignored.");
            continue;
        }
        if (*skip_spaces(input) == '\0') {
            continue;
        }
        if (strcmp(input, "exit") == 0) {
            break;
        }
        if (strcmp(input, "help") == 0) {
            print_help();
            continue;
        }
        if (strcmp(input, "history") == 0) {
            print_history(history, count);
            continue;
        }

        /* Only a complete calc command prefix selects the calculator tool. */
        if (strncmp(input, "calc", 4) == 0 &&
            (input[4] == '\0' || isspace((unsigned char)input[4]))) {
            calculator(input + 4, response);
        } else {
            mock_model(input, history, count, response);
        }

        printf("Assistant: %s\n", response);
        save_turn(history, &count, input, response);
    }

    puts("Bye!");
    return EXIT_SUCCESS; /* Automatic arrays are released on return. */
}
