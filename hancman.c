#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "regex.h"
#include <omp.h>
#ifndef NUM_GAMES
#define NUM_GAMES 256
#endif
#define HEADER "WORD,GUESS #,LETTER,DATA,STRATEGY,CORRECT"
typedef uint32_t LSet; // Letter Set
typedef struct Game Game;
typedef struct Guess Guess;
typedef struct GameRec GameRec;
char *dict;
size_t totalWords;
size_t readDict();
Game humanHost();
Game botHost();
int compare(const void *a, const void *b)
{
    if (*(size_t *)a < *(size_t *)b)
        return -1;
    if (*(size_t *)a > *(size_t *)b)
        return 1;
    return 0;
}
void botHostMany(Game *gameArr, size_t count);
GameRec playHuman(Game g);
void playBotProb(Game g, bool print, GameRec *rec);
Guess guessProb(Game *g, char *dict, bool print, char *newDict);
void playBotEntr(Game g, bool print, GameRec *rec);
Guess guessEntr(Game *g, char *dict, bool print, char *newDict);
static inline double binaryEntropy(double p)
{
    return -p * log2(p) - (1 - p) * log2(1 - p);
}
void writeOccurances(size_t *arr, LSet guessed, int wordLen, char *dict, char *newDict, regex_t *re, size_t *numWords);
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
static inline bool lSubset(LSet A, LSet B)
{
    return (A & B) == A;
}
LSet strToLSet(char *str);
void lSetToStr(char *dest, LSet l);
void printBlanks(char *word, LSet guessed);
void printLSet(LSet s);
struct Game
{
    char word[16];
    int lives, wordLen, numGuesses;
    LSet right, guessed;
    bool done;
};
void drawGame(Game g);
Game newGame(char *word);
Game partialGameFromBlanks(char *currBlanks, LSet guessed, int numGuesses);
static inline bool isPartialGame(Game g){
    return !g.done && lSubset(g.right, g.guessed);
}
bool guess(Game *g, char l);
char getGuessInput(LSet guessed);
enum Strategies
{
    HUMAN,
    PROBABILITY,
    ENTROPY
};
const char *STRAT_NAMES[3] = {"HUMAN", "PROBABILITY", "ENTROPY"};
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
void printGameRec(GameRec r);
void saveGameRec(GameRec r, char *fileName);
void saveGameRecs(GameRec *r, size_t count, char *fileName);
static inline LSet getWrongs(Game g)
{
    return g.guessed & ~g.right;
}
size_t readDict()
{
    if (dict != NULL)
        return totalWords;
    FILE *f = fopen("dictionary.txt", "r");
    fseek(f, 0, SEEK_END);
    dict = malloc(ftell(f));
    rewind(f);
    char word[16];
    while (fgets(word, 16, f) != NULL)
    {
        strncat(dict, word, 16);
        ++totalWords;
    }
    fclose(f);
    return totalWords;
}
Game humanHost()
{
    char wordInput[16];
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
    char word[20];
    wordIndex = ((((size_t)rand()) << 32) | rand()) % totalWords;
    for (size_t i = 0; i < wordIndex; i++)
        while (!fscanf(f, "%15s", word))
            ;
    fclose(f);
    // printf("Out of %ld words, #%ld was picked: %s", words, wordIndex, word);

    return newGame(word);
}
void botHostMany(Game *gameArr, size_t count)
{
    srand(clock());
    FILE *f = fopen("dictionary.txt", "r");
    readDict();
    size_t wordIndices[count], wordIndexMax = 0, currGame = 0;
    char word[20];
    for (size_t i = 0; i < count; i++)
    {
        wordIndices[i] = ((((size_t)rand()) << 32) | rand()) % totalWords;
        if (wordIndices[i] > wordIndexMax)
            wordIndexMax = wordIndices[i];
    }
    qsort(wordIndices, count, sizeof(size_t), &compare);
    for (size_t i = 0; i <= wordIndexMax; i++)
    {
        while (!fscanf(f, "%15s", word))
            ;
        if (i == wordIndices[currGame])
        {
            gameArr[currGame] = newGame(word);
            currGame++;
        }
    }
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
void playBotProb(Game g, bool print, GameRec *rec)
{
    readDict();
    char dictPre[totalWords * (g.wordLen + 1)], dictPost[totalWords * (g.wordLen + 1)];
    if (print)
        drawGame(g);
    rec->guesses[0] = guessProb(&g, dict, print, dictPre);
    for (int i = 1; !g.done; i++)
    {
        if (print)
            drawGame(g);
        rec->guesses[i] = guessProb(&g, i % 2 ? dictPre : dictPost, print, i % 2 ? dictPost : dictPre);
    }
    if (print)
    {
        drawGame(g);
        printf("You %s after %d guesses! The word was: %s", g.lives > 0 ? "won" : "lost", g.numGuesses, g.word);
    }
    rec->final = g;
}
Guess guessProb(Game *g, char *dict, bool print, char *newDict)
{
    size_t occurs[26], numWords = 0;
    double currProb, maxProb = 0;
    char maxL = 'A';
    regex_t pat;
    compPattern(&pat, *g);
    memset(occurs, 0, sizeof(occurs));
    writeOccurances(occurs, g->guessed, g->wordLen, dict, newDict, &pat, &numWords);
    if (print)
        printf("%lu words\n", numWords);
    for (int i = 0; i < 26; i++)
    {
        if (occurs[i] == 0)
            continue;
        currProb = occurs[i] / (double)numWords;
        if (print)
            printf("%c: %lu, %.17g| ", 'A' + i, occurs[i], currProb);
        if (currProb > maxProb)
        {
            maxL = 'A' + i;
            maxProb = currProb;
        }
    }
    if (print)
        printf("Max: %c(%.17g)\n", maxL, maxProb);
    return (Guess){*g, maxL, maxProb, PROBABILITY, guess(g, maxL)};
}
void playBotEntr(Game g, bool print, GameRec *rec)
{
    readDict();
    char dictPre[totalWords * (g.wordLen + 1)], dictPost[totalWords * (g.wordLen + 1)];
    if (print)
        drawGame(g);
    rec->guesses[0] = guessEntr(&g, dict, print, dictPre);
    for (int i = 1; !g.done; i++)
    {
        if (print)
            drawGame(g);
        rec->guesses[i] = guessEntr(&g, i % 2 ? dictPre : dictPost, print, i % 2 ? dictPost : dictPre);
    }
    if (print)
    {
        drawGame(g);
        printf("You %s after %d guesses! The word was: %s", g.lives > 0 ? "won" : "lost", g.numGuesses, g.word);
    }
    rec->final = g;
}
Guess guessEntr(Game *g, char *dict, bool print, char *newDict)
{
    size_t occurs[26], numWords = 0;
    double currProb, currEnt, maxEnt = 0;
    char maxL = 'A';
    regex_t pat;
    compPattern(&pat, *g);
    memset(occurs, 0, sizeof(occurs));
    writeOccurances(occurs, g->guessed, g->wordLen, dict, newDict, &pat, &numWords);
    if (print)
        printf("%lu words\n", numWords);
    for (int i = 0; i < 26; i++)
    {
        if (occurs[i] == 0)
            continue;
        currProb = occurs[i] / (double)numWords;
        currEnt = binaryEntropy(currProb);
        if (print)
            printf("%c: Occurance(s): %lu  Proability: %.17g  Entropy: %.17g\n", 'A' + i, occurs[i], currProb, currEnt);
        if (currEnt > maxEnt || currProb == 1)
        {
            maxL = 'A' + i;
            maxEnt = currEnt;
        }
    }
    if (print)
        printf("Max: %c(%.17g)\n", maxL, maxEnt);
    return (Guess){*g, maxL, maxEnt, ENTROPY, guess(g, maxL)};
}
void writeOccurances(size_t *arr, LSet guessed, int wordLen, char *dict, char *newDict, regex_t *re, size_t *numWords)
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
            break;
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
        if (numWords)
            (*numWords)++;
        dict += match.rm_so + wordLen;
    }
}
int compPattern(regex_t *dest, Game g)
{
    char negLetters[26], neg[26 + 3];
    int negLen;
    if (getWrongs(g) == (LSet)0)
        strcpy(neg, ".");
    else
    {
        lSetToStr(negLetters, getWrongs(g));
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
    Game g = {.lives = 6, .wordLen = 0, .numGuesses = 0, .right = (LSet)0, .guessed = (LSet)0, .done = false};
    for (int i = 0; word[i]; i++)
    {
        g.word[i] = word[i];
        g.wordLen++;
        toggL(&g.right, toupper(word[i]));
    }
    return g;
}
Game partialGameFromBlanks(char *currBlanks, LSet guessed, int numGuesses)
{
    Game g = {.lives = 6-numGuesses, .wordLen = 0, .numGuesses = numGuesses, .right = (LSet)0, .guessed = guessed, .done = true};
    for (int i = 0; currBlanks[i]; i++)
    {
        g.word[i] = currBlanks[i];
        g.wordLen++;
        if (isalpha(currBlanks[i]))
        {
            toggL(&g.right, toupper(currBlanks[i]));
        }else if(g.done){
            g.done = false;
        }
    }
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
void saveGameRec(GameRec r, char *fileName)
{
    Guess gs;
    FILE *f = fopen(fileName, "a+");
    char buf[strlen(HEADER)];
    if (fgets(buf, strlen(HEADER), f) != NULL)
    {
        if (strncmp(HEADER, buf, strlen(HEADER)))
            fprintf(f, "%s\n", HEADER);
    }
    fseek(f, 0, SEEK_END);
    for (int i = 0; i < r.final.numGuesses; i++)
    {
        gs = r.guesses[i];
        fprintf(f, "%s,%d,%c,%.17g,%s,%c\n", r.final.word, i, gs.letter, gs.data, STRAT_NAMES[gs.strat], gs.right ? 'Y' : 'N');
    }
}
void saveGameRecs(GameRec *recs, size_t count, char *fileName)
{
    GameRec r = recs[0];
    Guess gs = r.guesses[0];
    FILE *f = fopen(fileName, "a+");
    size_t headerLen = strlen(HEADER) + 1;
    char buf[headerLen];
    if (fgets(buf, headerLen, f) == NULL || strncmp(HEADER, buf, headerLen))
    {
        printf("%s %d", buf, strncmp(HEADER, buf, headerLen));
        fprintf(f, "%s\n", HEADER);
    }
    for (size_t i = 0; i < count; i++)
    {
        r = recs[i];
        if (r.guesses[0].data == 0)
        {
            printf("failed on %lu", i);
            printGameRec(r);
        }
        for (int j = 0; j < r.final.numGuesses; j++)
        {
            gs = r.guesses[j];
            fprintf(f, "%s,%d,%c,%.17g,%s,%c\n", r.final.word, j, gs.letter, gs.data, STRAT_NAMES[gs.strat], gs.right ? 'Y' : 'N');
        }
    }
    fclose(f);
}
Game games[NUM_GAMES];
GameRec recs[NUM_GAMES * 2];
int main()
{
    botHostMany(games, NUM_GAMES);
#pragma omp parallel for
    for (int i = 0; i < NUM_GAMES; i++)
    {
        playBotEntr(games[i], false, &recs[i]);
        if (recs[i].guesses[0].data == 0)
            printf("uh oh %d\n", i);
        playBotProb(games[i], false, &recs[NUM_GAMES + i]);
    }
    saveGameRecs(recs, NUM_GAMES * 2, "currOut.csv");
    free(dict);
    return 0;
}