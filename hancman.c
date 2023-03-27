#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
typedef uint32_t LSet; // Letter Set
void play2();
void drawMan(int lives);
bool getL(LSet set, char l);
void toggL(LSet *set, char l);
bool setL(LSet *set, char l, bool val);
bool lSubset(LSet A, LSet B);
LSet strToLSet(char *str);
void printBlanks(char *word, LSet guessed);
void printLSet(LSet s);
void play2()
{
    char word[16], guess = 0;
    int lives = 6;
    LSet right, guessed = 0;
    bool done = false;
    printf("\nPlaying 2-Player!\nEXECUTIONER, input a word: \e[8m");
    scanf("%15s", word);
    right = strToLSet(word);
    printf("\e[2K\e[28mEXECUTIONER entered a word!\n");
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
    while (!done)
    {
        guess = 0;
        printf("%d lives", lives);
        drawMan(lives);
        printBlanks(word, guessed);
        printLSet(guessed);
        while (!guess)
        {
            printf("RESCUER, guess a letter: ");
            scanf(" %c", &guess);
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
        guess = toupper(guess);
        toggL(&guessed, guess);
        if(!getL(right, guess)){
            if(--lives <= 0){
                done = true;
            }
        }else if(lSubset(right, guessed)) done = true;
    }
    drawMan(lives);
    printBlanks(word, guessed);
    printLSet(guessed);
    printf("You %s! The word was: %s", lives>0?"won":"lost", word);
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
bool getL(LSet set, char l)
{
    return (set >> (l - 65)) & 1;
}
void toggL(LSet *set, char l)
{
    *set ^= 1 << (l - 65);
}
bool setL(LSet *set, char l, bool val)
{
    if (val != 0 && val != 1)
        return false;
    if (getL(*set, l) != val)
        toggL(set, l);
    return true;
}
bool lSubset(LSet A, LSet B){
    return A & B == A;
}
LSet strToLSet(char *str)
{
    LSet s = 0;
    for (char *l = str; *l != '\0'; l++)
        setL(&s, toupper(*l), 1);
    return s;
}
void printBlanks(char *word, LSet guessed)
{
    printf("\n");
    for (int i = 0; i < strlen(word); i++)
    {
        printf("%c ", getL(guessed, word[i]) ? word[i] : '_');
    }
    printf("\n");
}
void printLSet(LSet s)
{
    LSet curr = s;
    char l;
    printf("\n");
    for (int i = 0; i < 26; i++)
    {
        l = i + 65;
        if (getL(s, l))
            printf("\e[2m");
        printf("%c\e[22m ", l);
    }
    printf("\n");
}
int main(int argc, char *argv[])
{
    play2();
    printf("\n");
    return 0;
}