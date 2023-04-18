#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "regex.h"
typedef uint32_t LSet; // Letter Set
typedef struct Game Game;
typedef struct Guess Guess;
typedef struct GameRec GameRec;
char *dict;
size_t numWords;
size_t readDict();
Game humanHost();
Game botHost();
GameRec playHuman(Game g);
GameRec playBotProb(Game g, bool print);
Guess guessProb(Game *g, char *dict, bool print, char *newDict);
GameRec playBotInfo(Game g, bool print);
static inline double binaryEntropy(double p)
{
    return -p * log2(p) - (1 - p) * log2(1 - p);
}
void writeOccurances(size_t *arr, LSet guessed, int wordLen, char *dict, char *newDict, regex_t *re);
int compPattern(regex_t *dest, Game g);
void drawMan(int lives);
static inline bool getL(LSet set, char l) // Returns whether l is in set
{
    return (set >> (l - 'A')) & 1;
}
static inline void toggL(LSet *set, char l) // If set[l] is 1, switch to 0 and vice versa
{
    *set ^= 1 << (l - 'A');
}
static inline bool setL(LSet *set, char l, bool val) // Sets set[l] to val
{
    if (getL(*set, l) != val)
    {
        toggL(set, l);
        return true;
    }
    return false;
}
bool lSubset(LSet A, LSet B);
LSet strToLSet(char *str);
void lSetToStr(char *dest, LSet l);
void printBlanks(char *word, LSet guessed);
void printLSet(LSet s);
struct Game
{
    char *word;
    int lives, wordLen, numGuesses;
    LSet right, guessed, wrongs;
    bool done;
};
void drawGame(Game g);
Game newGame(char *word);
bool guess(Game *g, char l);
char getGuessInput(LSet guessed);
enum Strategies
{
    HUMAN,
    PROBABILITY,
    ENTROPY
};
char *STRAT_NAMES[3] = {"HUMAN", "PROBABILITY", "ENTROPY"};
struct Guess
{
    Game state;
    char letter;
    double data;
    enum Strategies strat;
    bool right;
};
struct GameRec
{
    Guess guesses[26];
    Game final;
};
size_t readDict()
{
    if (dict != NULL)
        return numWords;
    FILE *f = fopen("dictionary.txt", "r");
    fseek(f, 0, SEEK_END);
    dict = malloc(ftell(f));
    rewind(f);
    static char word[20];
    while (fgets(word, 20, f) != NULL)
    {
        strncat(dict, word, 16);
        ++numWords;
    }
    fclose(f);
    return numWords;
}
Game humanHost()
{
    static char wordInput[16];
    printf("\nPlaying 2-Player!\nEXECUTIONER, input a word: \033[8m");
    while (!scanf("%15s", wordInput))
        ;
    printf("\033[2K\033[28mEXECUTIONER entered a word!\n");
    return newGame(wordInput);
}
Game botHost()
{
    srand(clock());
    FILE *f = fopen("dictionary.txt", "r");
    readDict();
    size_t wordIndex;
    static char word[20];
    wordIndex = ((((size_t)rand()) << 32) | rand()) % numWords;
    for (unsigned int i = 0; i < wordIndex; i++)
        while (!fscanf(f, "%15s", word))
            ;
    fclose(f);
    // printf("Out of %ld words, #%ld was picked: %s", words, wordIndex, word);

    return newGame(word);
}
GameRec playHuman(Game g)
{
    /*
    printf("(it was %s)", word);
    printLSet(strToLSet(word));
    printLSet((LSet)0b00000000000000000000000000010101);
    printBlanks(word, guessed);
    printLSet(guessed);
    toggL(&guessed, 'A');
    printBlanks(word, guessed);
    printLSet(guessed);
    toggL(&guessed, 'H');
    printBlanks(word, guessed);
    printLSet(guessed);
    toggL(&guessed, 'Z');
    printBlanks(word, guessed);
    printLSet(guessed);
    toggL(&guessed, 'B');
    printBlanks(word, guessed);
    printLSet(guessed);
    */
    GameRec rec;
    char l;
    for (int i = 0; !g.done; i++)
    {
        drawGame(g);
        l = getGuessInput(g.guessed);
        rec.guesses[i] = (Guess){g, l, 0, HUMAN, guess(&g, l)};
    }
    drawGame(g);
    printf("You %s after %d guesses! The word was: %s", g.lives > 0 ? "won" : "lost", g.numGuesses, g.word);
    rec.final = g;
    return rec;
}
GameRec playBotProb(Game g, bool print)
{
    GameRec rec;
    readDict();
    char dictPre[numWords * (g.wordLen + 1)], dictPost[numWords * (g.wordLen + 1)];
    rec.guesses[0] = guessProb(&g, dict, print, dictPre);
    for (int i = 1; !g.done; i++)
    {
        rec.guesses[i] = guessProb(&g, i % 2 ? dictPre : dictPost, print, i % 2 ? dictPost : dictPre);
    }
    // for(int i=0; !g.done; i++) rec.guesses[i] = guessProb(&g, dict, print, NULL);
    if (print)
    {
        drawGame(g);
        printf("You %s after %d guesses! The word was: %s", g.lives > 0 ? "won" : "lost", g.numGuesses, g.word);
    }
    rec.final = g;
    return rec;
}
Guess guessProb(Game *g, char *dict, bool print, char *newDict)
{
    size_t maxN;
    char maxL;
    regex_t pat;
    size_t occurs[26];
    compPattern(&pat, *g);
    if (print)
        drawGame(*g);
    memset(occurs, 0, sizeof(occurs));
    writeOccurances(occurs, g->guessed, g->wordLen, dict, newDict, &pat);
    maxL = 'A';
    maxN = occurs[0];
    for (int i = 0; i < 26; i++)
    {
        if (print)
            printf("%c: %lu | ", 'A' + i, occurs[i]);
        if (occurs[i] > maxN)
        {
            maxL = 'A' + i;
            maxN = occurs[i];
        }
    }
    if (print)
        printf("Max: %c(%lu)\n", maxL, maxN);
    return (Guess){*g, maxL, maxN, PROBABILITY, guess(g, maxL)};
}
GameRec playBotInfo(Game g, bool print)
{
    readDict();
    char maxL;
    regex_t pat;
    compPattern(&pat, g);
    size_t occurs[26];
    double currEnt, maxEnt, occursT;
    GameRec rec;
    for (int i = 0; !g.done; i++)
    {
        if (print)
            drawGame(g);
        memset(occurs, 0, sizeof(occurs));
        writeOccurances(occurs, g.guessed, g.wordLen, dict, NULL, &pat);
        maxL = 'A';
        maxEnt = 0;
        occursT = 0;
        for (int i = 0; i < 26; i++)
            occursT += occurs[i];
        for (int i = 0; i < 26; i++)
        {
            if (occurs[i] == 0)
                continue;
            currEnt = binaryEntropy(occurs[i] / occursT);
            if (print)
                printf("%c: Occurance(s): %lu  Proability: %f  Entropy: %f\n", 'A' + i, occurs[i], occurs[i] / occursT, currEnt);
            if (currEnt > maxEnt || occurs[i] / occursT == 1)
            {
                maxL = 'A' + i;
                maxEnt = currEnt;
            }
        }
        if (print)
            printf("Max: %c(%f)\n", maxL, maxEnt);
        rec.guesses[i] = (Guess){g, maxL, maxEnt, ENTROPY, guess(&g, maxL)};
        compPattern(&pat, g);
    }
    if (print)
    {
        drawGame(g);
        printf("You %s after %d guesses! The word was: %s", g.lives > 0 ? "won" : "lost", g.numGuesses, g.word);
    }
    rec.final = g;
    return rec;
}
void writeOccurances(size_t *arr, LSet guessed, int wordLen, char *dict, char *newDict, regex_t *re)
{
    LSet currSet;
    regmatch_t match = {.rm_so = 0};
    char currWord[wordLen + 1];
    int regErr = 0;
    if (newDict)
        strcpy(newDict, "");
    while (!regErr)
    {
        if ((regErr = regexec(re, dict, 1, &match, 0)) != 0)
            return;
        strncpy(currWord, dict + match.rm_so, wordLen);
        currWord[wordLen] = '\0';
        if (newDict)
        {
            strcat(newDict, currWord);
            strcat(newDict, "\n");
        }
        // currSet = strToLSet(currWord);
        // for (int i = 0; i < 26; i++)
        // {
        //     if ((currDiff = (currSet & (~guessed)) >> (i)))
        //         arr[i] += currDiff & 1;
        //     else break;
        // }
        currSet = 0;
        for (int i = 0; currWord[i]; i++)
        {
            arr[currWord[i] - 'A'] += (setL(&currSet, currWord[i], 1) & !getL(guessed, currWord[i]));
        }
        dict += match.rm_so + wordLen;
    }
}
int compPattern(regex_t *dest, Game g)
{
    char negLetters[26], neg[26 + 3];
    int negLen;
    if (g.wrongs == (LSet)0)
        strcpy(neg, ".");
    else
    {
        lSetToStr(negLetters, g.wrongs);
        sprintf(neg, "[^%s]", negLetters);
    }
    negLen = strlen(neg);
    char pat[g.wordLen * negLen + 1 * 2 + 1];
    strcpy(pat, "^");
    for (int i = 0; i < g.wordLen; i++)
    {
        if (getL(g.guessed, g.word[i]))
        {
            strncat(pat, g.word + i, 1);
        }
        else
        {
            strcat(pat, neg);
        }
    };
    strcat(pat, "$\0");
    // printf("\n|%s|\n", pat);
    return regcomp(dest, pat, REG_NEWLINE | REG_ICASE | REG_EXTENDED);
}
void drawMan(int lives)
{
    char *man[6] = {
        lives < 6 ? "O" : " ",
        lives < 5 ? "|" : " ",
        lives < 4 ? "/" : " ",
        lives < 3 ? "\\" : " ",
        lives < 2 ? "/" : " ",
        lives < 1 ? "\\" : " "};
    printf("\n");
    printf("┌━━┐\n");
    printf("│  ╽\n");
    printf("│  %s\n", man[0]);
    printf("│ %s%s%s\n", man[4], man[1], man[5]);
    printf("┷ %s %s\n", man[2], man[3]);
}
void drawGame(Game g)
{
    drawMan(g.lives);
    printBlanks(g.word, g.guessed);
    printLSet(g.guessed);
}
Game newGame(char *word)
{
    Game g = {.word = word, .lives = 6, .wordLen = strlen(word), .numGuesses = 0, .right = strToLSet(word), .guessed = (LSet)0, .wrongs = (LSet)0, .done = false};
    return g;
}
bool guess(Game *g, char l)
{
    l = toupper(l);
    if (!getL(g->guessed, l))
    {
        toggL(&(g->guessed), toupper(l));
        if (!getL(g->right, l))
        {
            setL(&(g->wrongs), l, 1);
            if (--g->lives <= 0)
            {
                g->done = true;
            }
        }
        else if (lSubset(g->right, g->guessed))
            g->done = true;
        g->numGuesses++;
    }
    return getL(g->right, l);
}
char getGuessInput(LSet guessed)
{
    char guess = 0;
    while (!guess)
    {
        printf("RESCUER, guess a letter: ");
        while (!scanf(" %c", &(guess)))
            ;
        // printf("%c", guess);
        if (!isalpha(guess))
        {
            printf("Not a letter! ");
            guess = 0;
        }
        else if (getL(guessed, guess))
        {
            printf("Already guessed! ");
            guess = 0;
        }
    }
    return guess;
}

bool lSubset(LSet A, LSet B)
{
    return (A & B) == A;
}
LSet strToLSet(char *str)
{
    LSet s = 0;
    char l;
    while ((l = *(str++)))
        setL(&s, toupper(l), 1);
    return s;
}
void lSetToStr(char *dest, LSet l)
{
    for (char i = 'A'; i <= 'Z'; i++)
    {
        if (getL(l, i))
            *(dest++) = i;
    }
    *dest = '\0';
}
void printBlanks(char *word, LSet guessed)
{
    printf("\n");
    for (unsigned int i = 0; i < strlen(word); i++)
    {
        printf("%c ", getL(guessed, word[i]) ? word[i] : '_');
    }
    printf("\n");
}
void printLSet(LSet s)
{
    char l;
    printf("\n");
    for (int i = 0; i < 26; i++)
    {
        l = i + 65;
        if (getL(s, l))
            printf("\033[2m");
        printf("%c\033[22m ", l);
    }
    printf("\n");
}
void printGameRec(GameRec r)
{
    Guess gs;
    printf("\nLETTER\t|DATA\t\t|STRAT\t\t|IN \"%s\"\n", r.final.word);
    for (int i = 0; i != r.final.numGuesses; i++)
    {
        gs = r.guesses[i];
        printf("%c\t|%8f\t|%-8s\t|%s\n", gs.letter, gs.data, STRAT_NAMES[gs.strat], gs.right ? "Y" : "N");
    }
}
int main()
{
    for (int i = 0; i < 100; i++)
        playBotProb(botHost(), false);
    free(dict);
    return 0;
}