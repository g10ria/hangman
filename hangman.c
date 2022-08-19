#include "./headers/hangman.h"

/**
 * Hangman guesser written for Info Theory (20-21)
 * @created 01/01/20
 * 
 * Todos [low priority]:
 * - Add input validation
 */

/* ----- MACROS ----- */

#define DEBUG 1        // set to 1 to print debug messages, 0 otherwise
#define ALPH_LENGTH 27 // length of the alphabet

#define WORDS_LIST_FILENAME "words/longlist.txt"
#define WORDS_SHORTLIST_FILENAME "words/shortlist.txt"
#define MAX_PHRASE_LENGTH 1024

/* ----- GLOBAL VARIABLES ----- */

int numGuesses;            // for printing
char guesses[ALPH_LENGTH]; // for printing
int guessHit[ALPH_LENGTH]; // set to -1 for miss, 1 for hit

int lieFound; // if a lie has been found
int lieIndex; // index of the lie (in the guesses) - also for printing

/**
     * alphabet:
     * 
     * 0 = unguessed
     * 1 = hit
     * 2 = missed (confirmed)
     * 3 = missed (unconfirmed)
     * 4 = must have (don't know where)
     * 
     */
int alphabet[ALPH_LENGTH]; // status of each letter (hit, unconfirmed miss, confirmed miss, etc.)

char *wordsList;       // contains all the word possibilities

int numWords;              // total number of words
int *numWordsOfEachLength; // number of words of each length
int *wordLengths;          // lengths of each word (user-provided)
int wordMaxLength;         // max length of word in phrase (user-provided)
int maxWordsOverLengths;   // max # of possibilities for a word length

char * latestValidCombo;

int totalPossibs; // total # of possibilities (permutations of word possibilities)

char *currLetters; // the current "template"

int phraseLength;

int numUnconfirmedMisses;

// if the program is currently
int testingLiePossibility;
// the character that must be there
char mustHaveLie;

int testingHitPossibility;
char mustHaveHit;

int gameOver;

int totalValidCombos = 0;

int specificValidCombos = 0;

/* ----- FUNCTION DECLARATIONS ----- */

// UI functions
// Prints the welcome screen
int printWelcomeScreen();
// prints the board (basically the line of letters)
void printBoard();
// plays a game of hangman until forfeit or win
void playGame();
// prompts for user input about number and length of words, spaces, punctuation
// also allocates and sets currLetters
void promptTemplate();
// prompts the user with a guess
void promptGuess();
// prints all past guesses
void printGuesses();
// prompts for input from the user. they can press 1 to get the next guess or 2 to quit
int promptInput();

int getValidCombos();

// gets a letter guess
int getLetterGuess();

// init/data allocation functions
void init();
void populateWordsList();
void allocateCurrLetters();

// Entropy computation functions
double entropyNoLie();
double entropyWithLie();
double entropyOfStatus();

// helper functions
int ind2(int i, int j);
int ind3(int i, int j, int k);
char ch(int i, int j, int k);
char indexToChar(int index);
int charToIndex(char c);

/* ----- CODE ----- */

/**
 * Prints the welcome screen. According to the player's action,
 * plays the game or quits.
 */
int main()
{
    int inp1 = printWelcomeScreen();

    if (inp1 == 1)
        playGame();
    else if (inp1 == 2)
        return 0;
    else
    {
        printf("There was an error.\n");
        return 1;
    }

    return 0;

    // alphabet[2] = 3; // set c to unconfirmed miss
    // currLetters[0] = 'r';
    // numUnconfirmedMisses = 1;
    // printf("%f\n", entropyWithLie());
}

double probSomethingIsLie()
{
    if (numUnconfirmedMisses == 0) return 0;
    double temp = 1.0 / (numUnconfirmedMisses + 1);
    if (0.125 < temp)
        return 0.125;
    return temp;
}

double probThereAreNoLies()
{
    if (numUnconfirmedMisses == 0) return 1.0;
    return 1.0 - probSomethingIsLie() * numUnconfirmedMisses;
}

void playGame()
{
    init();

    promptTemplate();
    populateWordsList();

    int quitGame = 0;
    while (!gameOver && !quitGame)
    {
        printBoard();
        printGuesses();
        quitGame = promptInput(); // gets set to 1 when error
    }

    if (quitGame)
    {
        printf("Quit game at %d guesses.\n", numGuesses);
    }
    else
        printf("Game over in %d guesses.\n", numGuesses);

    return;
}

void promptTemplate()
{
    // todo: input validation here

    // read in number of words
    printf("Enter the number of words in your phrase.\n");
    scanf(" %d", &numWords);

    wordLengths = malloc(sizeof(int) * numWords);

    // read in word lengths
    for (int i = 0; i < numWords; i++)
    {
        printf("What is the length of word %d?\n", i + 1);
        scanf(" %d", wordLengths + i);
        // printf("Word %d has length %d\n", i+1, wordLengths + i);
    }

    // allocate space for currLetters
    phraseLength = 0;
    for (int i = 0; i < numWords; i++)
    {
        phraseLength += wordLengths[i];
        if (i > 0)
            phraseLength++; // add the space
    }
    currLetters = malloc(sizeof(char) * phraseLength + 1);
    latestValidCombo = malloc(sizeof(char) * phraseLength);

    printf("Enter your word/phrase, replacing the letters with 'X's (i.e. QUIET --> XXXXX).\n");
    scanf(" "); // consume the newline

    fgets(currLetters, MAX_PHRASE_LENGTH, stdin);

    printf("ok read\n");

    // replace Xs in currLetters with _
    for (int i = 0; i < phraseLength; i++)
    {
        if (currLetters[i] == 'X')
            currLetters[i] = '_';
    }

    printf("\n");

    return;
}

/**
 * Prints all previous guesses.
 * Prints the guessed character, if it was hit or miss, and if it was the lie (exposer) or not.
 */ 
void printGuesses()
{
    printf("Guesses so far: %d\n", numGuesses);
    for (int i = 0; i < numGuesses; i++)
    {
        printf("Guess %d: %c [%s] %s\n", i + 1, guesses[i], guessHit[i] == 1 ? "HIT" : "MISS", lieIndex == i ? "[LIE FOUND]" : "");
    }
    return;
}

/**
 * Prompts the user to either press 1 for the next guess or 2 to quit the game.
 */ 
int promptInput()
{
    printf("Press 1 for next guess.\nPress 2 to quit game.\n");

    int inp;
    scanf(" %d", &inp);

    while (inp != 1 && inp != 2)
    {
        printf("Bad input, try again.\n");
        scanf(" %d", &inp);
    }

    if (inp == 1)
        promptGuess();
    else if (inp == 2)
        return 1;

    return 0;
}

/**
 * Prints the welcome screen for the game
 * 
 * @return player's input
 */
int printWelcomeScreen()
{
    printf("\nWELCOME TO HANGMAN\n\n");
    printf("Press 1 to play new game.\nPress 2 to quit.\n\n");

    int inp;
    scanf(" %d", &inp);

    while (inp != 1 && inp != 2)
    {
        printf("Bad input, try again.\n");
        scanf(" %d", &inp);
    }

    return inp;
}

/**
 * Prints the "board" for the game. It's just like the whiteboard hangman dashed lines.
 */ 
void printBoard()
{
    printf("\n");
    for (int i = 0; i < phraseLength; i++)
    {
        printf("%c ", currLetters[i]);
    }

    printf("\n");
    return;
}

/**
 * Initializes important variables and data.
 */ 
void init()
{
    gameOver = 0;
    testingLiePossibility = 0;
    numGuesses = 0;
    lieFound = 0;
    lieIndex = -1;
    totalPossibs = 1;

    for (int i = 0; i < ALPH_LENGTH; i++) alphabet[i] = 0;

    // set apostrohpe to already-hit
    alphabet[26] = 1;
}

int getStrictlyValidCombos();
int strictlyTestCombination(int *indices);

int shouldGuessEntirePhrase() {
    testingLiePossibility = 0;
    testingHitPossibility = 0;

    int strictlyValidCombos = getStrictlyValidCombos();

    printf("Combos left: %d\n", strictlyValidCombos);

    return strictlyValidCombos == 1;
}

int getStrictlyValidCombos()
{
    int strictlyValidCombos = 0;

    int *indices;

    indices = malloc(sizeof(int) * numWords);

    for (int i = 0; i < numWords; i++)
        indices[i] = 0;
    indices[numWords - 1] = -1;

    char *currTested = malloc(sizeof(char) * phraseLength);

    for (int i = 0; i < totalPossibs; i++)
    {
        int overflow = 1;
        for (int j = numWords - 1; j >= 0; j--)
        {

            if (overflow)
                indices[j]++;
            if (indices[j] == numWordsOfEachLength[j])
            {
                indices[j] = 0;
            }
            else
                overflow = 0;
        }

        int testCombo = strictlyTestCombination(indices);

        if (testCombo)
        { // combination worked
            strictlyValidCombos++;
                    
            int curr = 0;
            for (int j = 0; j < numWords; j++)
            {
                for (int k = 0; k < wordLengths[j]; k++)
                {
                    // current character in the combination being tested
                    latestValidCombo[curr] = ch(j, indices[j], k);
                    // printf("%c", latestValidCombo[curr]);
                    curr++;
                }
                // printf(" ");
                latestValidCombo[curr] = ' ';
                curr++;
            }
            // printf("\n");

        }
    }

    free(indices);
    free(currTested);

    return strictlyValidCombos;
}

int strictlyTestCombination(int *indices)
{
    int curr = 0;
    char c;
    int hitAMiss[ALPH_LENGTH];
    for(int i=0;i<ALPH_LENGTH;i++) hitAMiss[i] = 0;

    for (int j = 0; j < numWords; j++)
    {
        for (int k = 0; k < wordLengths[j]; k++)
        {
            // current character in the combination being tested
            c = ch(j, indices[j], k);

            if (alphabet[charToIndex(c)] == 2) return 0; // hit a confirmed miss

            // if (alphabet[charToIndex(c)] == 3)
            // {
            //     hitAMiss[charToIndex(c)] = 1;
            // }

            // if the character doesn't match the template
            if (currLetters[curr] != '_' && ch(j, indices[j], k) != currLetters[curr]) return 0;
            
            // hit character in a non-designated spot
            if (alphabet[charToIndex(c)] == 1 && currLetters[curr] != c) return 0;

            curr++;
        }
        curr++;
    }
    // todo - put this back
    // int sumOfHitsOnMisses = 0;
    // for(int i=0;i<ALPH_LENGTH;i++) if(hitAMiss[i]) sumOfHitsOnMisses++;
    // if (lieFound) {
    //     if (sumOfHitsOnMisses>0) return 0;
    //     return 1;
    // } else {
    //     if (sumOfHitsOnMisses>1) return 0;
    // }
    return 1;
}

void promptGuess()
{
    if (shouldGuessEntirePhrase()) {
        printf("Your word/phrase is \"");
        for(int i=0;i<phraseLength;i++) {
            printf("%c", latestValidCombo[i]);
        }
        printf("\".\n");

        numGuesses++;

        gameOver = 1;
        return;
    }

    int guessInt = getLetterGuess();
    char guessChar = indexToChar(guessInt);

    guesses[numGuesses] = guessChar;

    printf("Guess: '%c'.\nEnter 1 for hit.\nEnter 2 for miss.\n", guessChar);

    int inp;
    scanf(" %d", &inp);

    while (inp != 1 && inp != 2)
    {
        printf("Bad input, try again.\n");
        scanf(" %d", &inp);
    }

    if (inp == 1) // hit
    {
        guessHit[numGuesses] = 1;

        if (alphabet[guessChar - 97] == 3) // missed (unconfirmed)
        {
            lieFound = 1;
            lieIndex = numGuesses;
            printf("Overrode original answer, lie was found.\n");
            // don't need to update the other misses variables
            // doesn't really matter
        }
        alphabet[guessChar - 97] = 1;

        printf("How many ocurrences of this letter are there?.\n");
        scanf(" %d", &inp);

        int loc;

        for (int i = 0; i < inp; i++)
        {
            printf("Enter the location of ocurrence %d.\n", i + 1);
            scanf(" %d", &loc);

            currLetters[loc - 1] = guessChar;
        }
    }
    else // miss
    {
        guessHit[numGuesses] = -1;

        if (alphabet[guessInt] == 3) // was unconfirmed
        {
            alphabet[guessInt] = 2; // now is confirmed
            // printf("Confirmed original answer, this was not a lie.\n");
        }
        else // was previously unguessed
        {
            if (lieFound)
                alphabet[guessInt] = 2;
            else
            {
                alphabet[guessInt] = 3;
                numUnconfirmedMisses++;
                // printf("incrementing here %d\n", numUnconfirmedMisses);
            }
        }
    }

    numGuesses++;

    return;
}

char indexToChar(int index)
{
    char indexToChar[ALPH_LENGTH] = {
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
        'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
        'y', 'z', '\''};
    return indexToChar[index];
}

int charToIndex(char c)
{
    switch(c) {
        case 'a': return 0;
        case 'b': return 1;
        case 'c': return 2;
        case 'd': return 3;
        case 'e': return 4;
        case 'f': return 5;
        case 'g': return 6;
        case 'h': return 7;
        case 'i': return 8;
        case 'j': return 9;
        case 'k': return 10;
        case 'l': return 11;
        case 'm': return 12;
        case 'n': return 13;
        case 'o': return 14;
        case 'p': return 15;
        case 'q': return 16;
        case 'r': return 17;
        case 's': return 18;
        case 't': return 19;
        case 'u': return 20;
        case 'v': return 21;
        case 'w': return 22;
        case 'x': return 23;
        case 'y': return 24;
        case 'z': return 25;
        case '\'': return 26; // apostrophe
    }
}

double probsOfHit(int i)
{
    double probOfLie = probSomethingIsLie();
    double probOfNoLies = probThereAreNoLies();

    double probOfHit = 0.0;

    testingLiePossibility = 0;

    testingHitPossibility = 1;
    mustHaveHit = indexToChar(i);
    int before = alphabet[i];
    alphabet[i] = 4;
    getValidCombos();
    alphabet[i] = before;

    // printf("lie probs for %d: %f %f\n",i, probOfLie, probOfNoLies);

    // printf("%d out of %d ", specificValidCombos, totalValidCombos);

    probOfHit += probOfNoLies * ((double)specificValidCombos) / totalValidCombos;
    
    testingLiePossibility = 1;

    for (int i = 0; i < ALPH_LENGTH; i++)
    {
        if (alphabet[i] == 3) // unconfirmed miss
        {
            alphabet[i] = 4;
            mustHaveLie = indexToChar(i);

            getValidCombos();

            // printf("%d out of %d ", specificValidCombos, totalValidCombos);
            probOfHit += probOfLie * ((double)specificValidCombos) / totalValidCombos;

            alphabet[i] = 3;
        }
    }

    testingLiePossibility = 0;
    testingHitPossibility = 0;

    return probOfHit;
}

// returns the index of the guess
int getLetterGuess() // this is a letter guess
{
    double minEntropy = DBL_MAX;
    int guessInt;

    if (!lieFound && numUnconfirmedMisses > 0)
    { // guess lies
    
        // printf("Now guessing lies\n");
        for (int i = 0; i < ALPH_LENGTH; i++)
        {
            if (alphabet[i] == 3) // missed, unconfirmed
            {
                double probsLie = probSomethingIsLie();

                testingLiePossibility = 1;
                mustHaveLie = indexToChar(i);
                alphabet[i] = 4;

                double lieEntropy = entropyNoLie();

                testingLiePossibility = 0;
                alphabet[i] = 2;

                double missEntropy = entropyWithLie();

                double totalEntropy = probsLie * lieEntropy + (1.0 - probsLie) * missEntropy;

                if (totalEntropy < minEntropy)
                {
                    minEntropy = totalEntropy;
                    guessInt = i;
                }

                alphabet[i] = 3;
            }
        }
    }


        // printf("Now guessing unguessed letters\n");

        for (int i = 0; i < ALPH_LENGTH; i++)
        {
            // printf("wtf\n");
            if (alphabet[i] == 0) // unguessed
            {
                // Probability that the guess is a hit
                double probsHit = probsOfHit(i);

                
                if (probsHit == 1.0) {
                    // printf("prob of hit was 1.0, returning early\n");
                    guessInt = i;
                    return guessInt;
                }
                else if (probsHit > 0)
                {
                    // printf("\tGuessing '%c'\n", indexToChar(i));
                    // printf("Prob of hit: %f\n", probsHit);



                    // Simulating entropy if the guess was a hit
                    testingHitPossibility = 1;
                    mustHaveHit = indexToChar(i);
                    alphabet[i] = 4;

                    double hitEntropy;
                    if (lieFound)
                        hitEntropy = entropyNoLie();
                    else
                        hitEntropy = entropyWithLie();
                    // printf("Entropy if hit: %f\n", hitEntropy);
                    testingHitPossibility = 0;



                    // Simulating entropy if the guess was a miss
                    if (lieFound)
                        alphabet[i] = 2;
                    else
                        alphabet[i] = 3;
                    if (!lieFound)
                        numUnconfirmedMisses++;

                    double missEntropy;
                    if (lieFound)
                        missEntropy = entropyNoLie();
                    else
                        missEntropy = entropyWithLie();

                    if (!lieFound)
                        numUnconfirmedMisses--;
                    // printf("Entropy if miss: %f\n", missEntropy);




                    // Total entropy
                    double totalEntropy = probsHit * hitEntropy + (1.0 - probsHit) * missEntropy;

                    // printf("Total entropy: %f\n", totalEntropy);

                    if (totalEntropy < minEntropy)
                    {
                        minEntropy = totalEntropy;
                        guessInt = i;
                    }

                    alphabet[i] = 0;
                }
            }
        }

    // printf("Best guess was %c with entropy %f\n", indexToChar(guessInt), minEntropy);
    return guessInt;
}

// returns 1 if the combination is valid, 0 otherwise
/**
 * Variables used:
 * testingLiePossibility
 * alphabet
 * currLetters (template)
 * 
 * Tests the following things:
 * - It has the "mustHaveLie" character
 * - It has the "mustHaveHit" character
 * - It doesn't contain any characters that were misses
 * - It doesn't have any characters that conflict with the "template"
 * - It doesn't have any hit characters in non-designated spots
 *  
 * testCombination returns 1 if all conditions are satisfied, 
 * 2 if conditions 3, 4, 5, are satisfied (only goes into total)
 * and 0 otherwise.
 * 
 */
int testCombination(int *indices)
{
    int hasLie = 0; // if we have the mustHaveLie or not
    int hasHit = 0; // if we have the mustHaveHit or not

    int curr = 0;
    char c;

    for (int j = 0; j < numWords; j++)
    {
        for (int k = 0; k < wordLengths[j]; k++)
        {
            // current character in the combination being tested
            c = ch(j, indices[j], k);
            // printf("%c is %d\n", c, c);

            // we have the mustHaveLie character
            if (testingLiePossibility && c == mustHaveLie)
            {
                // printf("we have mustHaveLie character %c\n", c);
                hasLie = 1;
            }

            if (testingHitPossibility && c == mustHaveHit) {
                hasHit = 1;
            }

            // if (c == 'z') printf("wtf is happening %d\n", alphabet[c - 97]);

            if (alphabet[c - 97] == 2 || alphabet[c - 97] == 3)
            {
                // printf("Returning 0\n");
                // printf("Had a missed char\n");
                // printf("2return 0\n");
                return 0;
            }

            // if the character doesn't match the template
            if (currLetters[curr] != '_' && ch(j, indices[j], k) != currLetters[curr])
            {
                // printf("Char doesn't match template\n");
                // printf("3return 0\n");
                return 0;
            }

            if (alphabet[c-97] == 1 && currLetters[curr] != c)
            {
                // printf("%c is hit but not in this spot\n", c);
                // if it's a hit letter but not in that spot
                return 0;
            }

            curr++;
        }
        curr++;
    }

    if (testingLiePossibility && !hasLie)
    {
        return 2;
    }
    if (testingHitPossibility && !hasHit) {
        return 2;
    }
    return 1;
}

double entropyNoLie()
{
    return entropyOfStatus(1.0);
    // takes the entropy of the current status; assumes the lie has
    // already been found and corrected
}

double entropyWithLie()
{
    if (numUnconfirmedMisses == 0 || lieFound)
        return entropyNoLie();

    double probOfLie = probSomethingIsLie();
    double probOfNotLie = probThereAreNoLies();

    // printf("%f chance of lying\n", probOfLie);

    double entropy = 0.0;

    testingLiePossibility = 0;

    double e = entropyOfStatus(probOfNotLie);
    // if (e != DBL_MAX) entropy += e;
    entropy += e;

    // printf("entropy before adding misses: %f\n", entropy);

    for (int i = 0; i < ALPH_LENGTH; i++)
    {
        if (alphabet[i] == 3)
        {

            alphabet[i] = 4; // set to must have
            mustHaveLie = indexToChar(i);
            testingLiePossibility = 1;

            e = entropyOfStatus(probOfLie);
            // if (e != DBL_MAX) entropy += e;
            entropy += e;

            testingLiePossibility = 0;
            alphabet[i] = 3;
        }
    }

    return entropy;
}

int getValidCombos()
{
    int validCombos = 0;

    totalValidCombos = 0;
    specificValidCombos = 0;

    int *indices;

    indices = malloc(sizeof(int) * numWords);

    for (int i = 0; i < numWords; i++)
        indices[i] = 0;
    indices[numWords - 1] = -1;

    char *currTested = malloc(sizeof(char) * phraseLength);

    for (int i = 0; i < totalPossibs; i++)
    {
        int overflow = 1;
        for (int j = numWords - 1; j >= 0; j--)
        {

            if (overflow)
                indices[j]++;
            if (indices[j] == numWordsOfEachLength[j])
            {
                indices[j] = 0;
            }
            else
                overflow = 0;
        }

        int testCombo = testCombination(indices);

        if (testCombo == 1)
        { // combination worked
            validCombos++;
            specificValidCombos++;
            totalValidCombos++;

        }
        else if (testCombo == 2)
        {
            totalValidCombos++;
        }
    }

    free(indices);
    free(currTested);

    return validCombos;
}

double entropyOfStatus(double probFactor)
{
    int validCombos = getValidCombos();
    // printf("valid combos: %d\n", validCombos);

    if (validCombos == 0) return DBL_MAX; // hm.

    return probFactor * log2((double)validCombos / probFactor);
}

/** Functions below this point not important */

void populateWordsList()
{
    printf("populating words list\n");
    // printf("now populating words list\n");
    FILE *wordsListFile;

    if (numWords == 1)
    {
        // printf("reading from long list\n");
        wordsListFile = fopen(WORDS_LIST_FILENAME, "r");
    }
    else
    {
        // printf("reading from short list\n");
        wordsListFile = fopen(WORDS_SHORTLIST_FILENAME, "r"); // use shortlist for phrases
        if (wordsListFile == NULL)
        {
            printf("oh no\n");
        }
    }

    numWordsOfEachLength = malloc(sizeof(int) * numWords);
    for (int i = 0; i < numWords; i++)
        numWordsOfEachLength[i] = 0;

    // get the max word length
    wordMaxLength = 0;
    for (int i = 0; i < numWords; i++)
        if (wordLengths[i] > wordMaxLength)
            wordMaxLength = wordLengths[i] + 1;

    printf("1");

    // get the max number of word possibilities per word
    int currentWordLength = 1;
    char c = fgetc(wordsListFile);
    while (c != EOF)
    {
        // printf("%c\n", c);
        currentWordLength++;

        c = fgetc(wordsListFile);

        if (c == '\n' || c == EOF)
        {
            for (int i = 0; i < numWords; i++) {
                if (currentWordLength - 1 == wordLengths[i]) {
                    numWordsOfEachLength[i]++;
                    // printf("num words for %d is now %d\n", wordLengths[i], numWordsOfEachLength[i]);
                }  
            }
                
            currentWordLength = 0;
        }
    }
    maxWordsOverLengths = 0; // = max # of words over the lengths we have
    for (int i = 0; i < numWords; i++)
    {
        if (DEBUG)
            printf("%d words with length %d\n", numWordsOfEachLength[i], wordLengths[i]);
        if (numWordsOfEachLength[i] > maxWordsOverLengths)
            maxWordsOverLengths = numWordsOfEachLength[i];
    }

    wordsList = malloc(numWords * maxWordsOverLengths * (wordMaxLength + 1) * sizeof(char));

    if (DEBUG)
        printf("Allocating %d bytes of memory\n", numWords * maxWordsOverLengths * (wordMaxLength + 1) * sizeof(char));

    rewind(wordsListFile);

    // actually populate the words
    int *numPopulatedWords = malloc(sizeof(int) * numWords);
    for (int i = 0; i < numWords; i++)
        numPopulatedWords[i] = 0;

    printf("max length is %d\n", wordMaxLength);
    char *tempWord = malloc(wordMaxLength * sizeof(char));
    currentWordLength = 0;
    c = fgetc(wordsListFile);

    while (c != EOF)
    {
        if (currentWordLength < wordMaxLength)
        {
            tempWord[currentWordLength] = c;
        }

        currentWordLength++;

        c = fgetc(wordsListFile);

        if (c == '\n' || c == EOF)
        {
            for (int i = 0; i < numWords; i++)
            {
                if (currentWordLength == wordLengths[i])
                {
                    // printf("adding word to length %d\n", currentWordLength);

                    int index = ind2(i, numPopulatedWords[i]);
                    for (int j = 0; j < currentWordLength; j++)
                    {
                        wordsList[index] = tempWord[j];
                        index++;
                    }

                    wordsList[index] = '\0'; // add null term
                    numPopulatedWords[i]++;
                }
            }

            currentWordLength = 0;
            c = fgetc(wordsListFile);
        }
    }

    if (DEBUG)
    {
        // printf("Reached the end\n");
        // for (int i = 0; i < numWords; i++)
        // {
        //     printf("Words with length %d:\n", wordLengths[i]);
        //     for (int j = 0; j < numWordsOfEachLength[i]; j++)
        //     {
        //         printf("%s\n", wordsList + ind2(i, j));
        //     }
        // }
    }

    for (int i = 0; i < numWords; i++)
    {
        totalPossibs *= numWordsOfEachLength[i];
    }

    printf("total possibs: %d\n\n", totalPossibs);

    free(numPopulatedWords);
    free(tempWord);

    fclose(wordsListFile);

    return;
}

char ch(int i, int j, int k)
{
    return wordsList[ind3(i, j, k)];
}

int ind2(int i, int j)
{
    return i * (maxWordsOverLengths * (wordMaxLength + 1)) + j * (wordMaxLength + 1);
}

int ind3(int i, int j, int k)
{
    return i * (maxWordsOverLengths * (wordMaxLength + 1)) + j * (wordMaxLength + 1) + k;
}