package hangman;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Scanner;

public class Game {

	final int ALPH_LENGTH = 27;
	final double MAX_LIE_PROB = 0.125;

	// for printing
	ArrayList<Character> guesses = new ArrayList<Character>();
	ArrayList<Integer> guessResults = new ArrayList<Integer>();
	final int G_HIT = 1;
	final int G_MISS = -1;

	boolean lieFound; // if a lie has been found
	int lieIndex; // index of the lie (in the guesses) - also for printing

	// letter status tracker
	int alphabet[] = new int[ALPH_LENGTH]; // status of each letter (hit, unconfirmed miss, confirmed miss, etc.)
	final int A_UNGUESSED = 0;
	final int A_HIT = 1;
	final int A_MISSED_CONFIRMED = 2;
	final int A_MISSED_UNCONFIRMED = 3;
	final int A_TEMP = 4;

	// contains all the word possibilities
	HashMap<Integer, ArrayList<String>> wordsList = new HashMap<Integer, ArrayList<String>>();
	
	int numWords; // total number of words
	int[] wordLengths; // lengths of each word (user-provided)
	int wordMaxLength; // max length of word in phrase (user-provided)
	int maxWordsOverLengths; // max # of possibilities for a word length
	int[] wordStartingIndices;

	String[] regexes;

	/**
	 * Initialize to length of phrase - Set to letter if hit letter - Set to
	 * C_UNGUESSED if unguessed blank - Set to ' ', ''', etc. for spaces,
	 * punctuation, etc.
	 */
	char[] overallStatus;
	final char C_UNGUESSED = '~';

	String[] latestValidCombo;

	int totalPossibs; // total # of possibilities (permutations of word possibilities)

	int phraseLength;

	int numUnconfirmedMisses;

	boolean gameOver;

	int[] totalValidWords;
	int[] specificValidWords;

	Scanner sc;

	public static void main(String[] args) {
		Game game = new Game();
		game.play();

		return;
	}

	int takeInput(int[] acceptedInputs) {
		int inp;
		inp = sc.nextInt();

		while (!contains(acceptedInputs, inp))
			inp = sc.nextInt();

		return inp;
	}

	boolean contains(int[] arr, int toCheckValue) {
		for (int i = 0; i < arr.length; i++) {
			if (arr[i] == toCheckValue)
				return true;
		}
		return false;
	}

	void printBoard() {
		pln("");
		for (int i = 0; i < phraseLength; i++) {
			if (overallStatus[i] == C_UNGUESSED)
				System.out.print("_ ");
			else
				System.out.print(overallStatus[i] + " ");
		}
		pln("");
		return;
	}

	void play() {
		init();

		pln("WELCOME TO HANGMAN");
		pln("Press 1 to play new game.");
		pln("Press 2 to quit.\n");

		int inp1 = takeInput(new int[] { 1, 2 });

		if (inp1 == 1)
			playGame();
		else if (inp1 != 2) {
			pln("There was an error.");
		}

		return;
	}

	void playGame() {
		init();

		promptTemplate();
		populateWordsList();

		boolean quitGame = false;
		while (!gameOver && !quitGame) {
			printBoard();
			printGuesses();
			quitGame = !promptInput(); // returns false if game is quit
		}

		if (quitGame)
			pln("Quit game at " + guesses.size() + " guesses.");
		else
			pln("Game over in " + guesses.size() + " guesses.\n");

		return;
	}

	void init() {
		sc = new Scanner(System.in);

		lieFound = false;
		lieIndex = -1;

		// set apostrophe to already-hit
		alphabet[26] = A_HIT;
	}

	void populateWordsList() {
		File file = new File("words/longlist.txt");
		FileReader fr;
		try {
			fr = new FileReader(file);
			BufferedReader br = new BufferedReader(fr); // creates a buffering character input stream

			String line;
			while ((line = br.readLine()) != null) {
				if (wordsList.containsKey(line.length()))
					wordsList.get(line.length()).add(line);
			}

			br.close();

		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	void promptTemplate() {
		// read in number of words
		pln("Enter the number of words in your phrase.");

		numWords = sc.nextInt();

		wordLengths = new int[numWords];
		specificValidWords = new int[numWords];
		totalValidWords = new int[numWords];
		latestValidCombo = new String[numWords];
		wordStartingIndices = new int[numWords];
		regexes = new String[numWords];

		// read in word lengths + calc phrase length
		for (int i = 0; i < numWords; i++) {
			pln("What is the length of word " + (i + 1) + "?");
			wordLengths[i] = sc.nextInt();

			if (i > 0)
				phraseLength++; // add the space
			wordStartingIndices[i] = phraseLength;
			phraseLength += wordLengths[i];
		}

		for (int i = 0; i < numWords; i++) {
			wordsList.put(wordLengths[i], new ArrayList<String>());
		}

		pln("Enter your word/phrase, replacing the letters with 'X's.");

		String temp = sc.nextLine(); // consume newline
		overallStatus = sc.nextLine().toCharArray();

		// replace Xs in currLetters with _
		for (int i = 0; i < phraseLength; i++) {
			if (overallStatus[i] == 'X')
				overallStatus[i] = C_UNGUESSED;
		}

		pln("");
	}

	double getTotalValidCombos() {
		double prod = 1;
		for (int i = 0; i < numWords; i++) {
			prod *= (double) totalValidWords[i];
			pln("valid combos for word "+(i+1)+": " + totalValidWords[i]);
		}
		return prod;
	}

	double getSpecificValidCombos() {
		double prod = 1;
		for (int i = 0; i < numWords; i++) {
			prod *= (double) specificValidWords[i];
		}
		return prod;
	}
	
	int getSpecificValidCombosSum() {
		int sum = 0;
		for(int i=0;i<numWords;i++) {
			sum += specificValidWords[i];
		}
		return sum;
	}
	
	int getTotalValidCombosSum() {
		int sum = 0;
		for(int i=0;i<numWords;i++) {
			sum += totalValidWords[i];
		}
		return sum;
	}

	boolean shouldGuessEntirePhrase() {
		getValidWords(new char[0]);
		double totalValidCombos = getTotalValidCombos();
		pln("Valid combos left: " + totalValidCombos);
		return totalValidCombos == 1;
	}

	void generateRegexes() {
		for (int i = 0; i < numWords; i++) {

			String regex = "";

			int startingIndex = wordStartingIndices[i];
			for (int j = 0; j < wordLengths[i]; j++) {
				char c = overallStatus[startingIndex + j];
				switch (c) {
				case C_UNGUESSED:
					regex += "[a-z]";
					break;
				default:
					regex += c;
				}
			}

			regexes[i] = regex;
		}
	}

	void promptGuess() {
		generateRegexes();

		if (shouldGuessEntirePhrase()) {

			pln("Your word/phrase is:");
			for (int i = 0; i < numWords; i++) {
				System.out.print(latestValidCombo[i] + " ");
			}
			pln("");

			guesses.add(' ');
			gameOver = true;

			return;
		} else if (guesses.size() > 0 && getTotalValidCombos() == 0) {
			pln("There are no possible phrases left. Please check your input or dictionary.");
			return;
		}

		int guessInt = getLetterGuess();
		char guessChar = indexToChar(guessInt);

		guesses.add(guessChar);

		pln("Guess: '" + guessChar + "'.\nEnter 1 for hit.\nEnter 2 for miss.\n");

		int inp = takeInput(new int[] { 1, 2 });

		if (inp == 1) // hit
		{
			guessResults.add(G_HIT);

			if (alphabet[charToIndex(guessChar)] == A_MISSED_UNCONFIRMED) {
				lieFound = true;
				lieIndex = guesses.size() - 1;
				pln("Overrode original answer, lie was found.\n");

				// update unconfirmed misses to be confirmed
				for (int i = 0; i < ALPH_LENGTH; i++) {
					if (alphabet[i] == A_MISSED_UNCONFIRMED)
						alphabet[i] = A_MISSED_CONFIRMED;
				}
				numUnconfirmedMisses = 0;
			}
			alphabet[charToIndex(guessChar)] = A_HIT;

			pln("How many ocurrences of this letter are there?.\n");
			int numOcurrences = sc.nextInt();

			int location;

			for (int i = 0; i < numOcurrences; i++) {
				pln("Enter the location of ocurrence " + (i + 1) + ".");
				location = sc.nextInt();

				overallStatus[location - 1] = guessChar;
			}
		} else // miss
		{
			guessResults.add(G_MISS);
			
			pln("miss: " + guessInt + " " + alphabet[guessInt]);

			if (alphabet[guessInt] == A_MISSED_UNCONFIRMED) // was unconfirmed
			{
				alphabet[guessInt] = A_MISSED_CONFIRMED;
			} else // was previously unguessed
			{
				if (lieFound)
					alphabet[guessInt] = A_MISSED_CONFIRMED;
				else {
					alphabet[guessInt] = A_MISSED_UNCONFIRMED;
					numUnconfirmedMisses++;
				}
			}
		}
		
		for(int i=0;i<ALPH_LENGTH;i++) {
			System.out.print(alphabet[i] + " ");
		}
		pln("");
	}

	void printGuesses() {
		int numGuesses = guesses.size();
		pln("Guesses so far: " + numGuesses);
		for (int i = 0; i < numGuesses; i++) {
			String guessResult = guessResults.get(i) == G_HIT ? "HIT" : "MISS";
			String ifLie = i == lieIndex ? " [LIE FOUND]" : "";
			pln("Guess " + (i + 1) + ": " + guesses.get(i) + " " + guessResult + ifLie);
		}
	}

	boolean promptInput() {
		pln("Press 1 for next guess.");
		pln("Press 2 to quit game");

		int inp = takeInput(new int[] { 1, 2 });

		if (inp == 1)
			promptGuess();
		else if (inp == 2)
			return false;

		return true;
	}

	void getValidWords(char[] neededChars) {
		for (int k = 0; k < numWords; k++) {
			totalValidWords[k] = 0;
			specificValidWords[k] = 0;

			ArrayList<String> wordList = wordsList.get(wordLengths[k]);
			int numWordsToTest = wordList.size();
			for (int i = 0; i < numWordsToTest; i++) {

				// hasAllNeededChars, hasNoMissedChars, hitOneOrLessMissedChar, matchesRegex,
				// hasNoMisplacedHitCharacters
				boolean[] res = testWord(k, wordList.get(i), neededChars);

//				pln("regex: " + regexes[k].toString());
//				pln("tested " + wordList.get(i)+": "+res[0]+" "+res[1]+" " +res[2] + " " +res[3] + " " + res[4]);

				if (lieFound) {
					// no more lie
					if (res[1] && res[2] && res[3] && res[4]) { // all except hasAllNeededChars
						totalValidWords[k]++;
						if (res[0]) {
							specificValidWords[k]++;

							latestValidCombo[k] = wordList.get(i);
						}
					}
				} else {
					if (res[2] && res[3] && res[4]) { // all except hasNoMissedChars and hasAllNeededChars
						totalValidWords[k]++;
						
						if (res[0]) { // hasAllNeededChars
							specificValidWords[k]++;

							latestValidCombo[k] = wordList.get(i);
						}
					}
				}
			}

		}
	}

	// tests a combination with the given indices
	boolean[] testWord(int wordIndex, String word, char[] neededChars) {
		int numNeededChars = neededChars.length;
		boolean hasAllNeededChars = true;
		boolean[] hitMissedChar = new boolean[ALPH_LENGTH];
		boolean hitOneOrLessMissedChar = true;
		boolean hasNoMissedChars = true;
		boolean matchesRegex = true;
		boolean hasNoMisplacedHitCharacters = true;

		// 1. test that it has all needed characters
		for (int i = 0; i < numNeededChars; i++) {
			if (word.indexOf(neededChars[i]) == -1)
				hasAllNeededChars = false;
		}

		// 2. test that it does not have any missed chars
		// 3. test that it does not hit more than one missed char
		int hitMissedChars = 0;
		for (int i = 0; i < ALPH_LENGTH; i++) {
			if (alphabet[i] == A_MISSED_UNCONFIRMED || alphabet[i] == A_MISSED_CONFIRMED) {
				if (word.indexOf(indexToChar(i)) != -1) {
					hasNoMissedChars = false;
					hitMissedChar[i] = true;

					// hitting confirmed miss is automatic DQ
					if (alphabet[i] == A_MISSED_CONFIRMED)
						hitMissedChars += ALPH_LENGTH;
				}
			}
		}
		for (int i = 0; i < ALPH_LENGTH; i++) {
			if (hitMissedChar[i])
				hitMissedChars++;
		}
		hitOneOrLessMissedChar = hitMissedChars <= 1;

		// 3. test that it matches the regex
		matchesRegex = word.matches(regexes[wordIndex]);

		// 4. test that it has no hit chars in wrong positions
		char c;
		int indexInOverallStatus = wordStartingIndices[wordIndex];
		for (int j = 0; j < word.length(); j++) {
			c = word.charAt(j);

			if (alphabet[charToIndex(c)] == A_HIT && overallStatus[indexInOverallStatus + j] == C_UNGUESSED) {
				hasNoMisplacedHitCharacters = false;
				break;
			}
		}

		boolean[] results = { hasAllNeededChars, hasNoMissedChars, hitOneOrLessMissedChar, matchesRegex,
				hasNoMisplacedHitCharacters };
		return results;
	}

	double probSomethingIsLie() {
		if (numUnconfirmedMisses == 0)
			return 0;
		double temp = 1.0 / (numUnconfirmedMisses + 1);
		if (MAX_LIE_PROB < temp)
			return MAX_LIE_PROB;
		return temp;
	}

	double probThereAreNoLies() {
		if (numUnconfirmedMisses == 0)
			return 1.0;
		return 1.0 - probSomethingIsLie() * numUnconfirmedMisses;
	}

	double probsOfHit(int i) {
		double probOfLie = probSomethingIsLie();
		double probOfNoLies = probThereAreNoLies();

		double probOfHit = 0.0;

		// todo

		// With no lie
		char[] neededChars = { indexToChar(i) };

		int a_before = alphabet[i];
		alphabet[i] = A_TEMP;

		getValidWords(neededChars);

		alphabet[i] = a_before;

		// printf("lie probs for %d: %f %f\n",i, probOfLie, probOfNoLies);
		// printf("%d out of %d ", specificValidCombos, totalValidCombos);

		probOfHit += probOfNoLies * ((double) getSpecificValidCombosSum()) / getTotalValidCombosSum();// todo fix this

		neededChars = new char[2];
		neededChars[0] = indexToChar(i);

		for (int j = 0; j < ALPH_LENGTH; j++) {
			if (alphabet[j] == A_MISSED_UNCONFIRMED) // unconfirmed miss
			{
				alphabet[j] = A_TEMP;
				neededChars[1] = indexToChar(j);

				getValidWords(neededChars);

				// printf("%d out of %d ", specificValidCombos, totalValidCombos);
				probOfHit += probOfLie * ((double) getSpecificValidCombosSum()) / getTotalValidCombosSum();

				alphabet[j] = A_MISSED_UNCONFIRMED;
			}
		}

		return probOfHit;
	}

	int getLetterGuess() {
		double minEntropy = Double.MAX_VALUE;
		int guessInt = 0;
		
		pln("generating move:");
		for(int i=0;i<ALPH_LENGTH;i++) {
			System.out.print(alphabet[i]);
		}
		pln("");

		generateRegexes();

		if (!lieFound && numUnconfirmedMisses > 0) { // guess lies

			char[] neededChars = new char[1];

			// pln("Now guessing lies\n");
			for (int i = 0; i < ALPH_LENGTH; i++) {
				if (alphabet[i] == A_MISSED_UNCONFIRMED) // missed, unconfirmed
				{
					double probsLie = probSomethingIsLie();

					neededChars[0] = indexToChar(i);

					// Found the lie
					alphabet[i] = A_TEMP;
					double lieEntropy = entropyNoLie(neededChars);

					// Didn't find the lie; it's now a confirmed miss
					alphabet[i] = A_MISSED_CONFIRMED;
					numUnconfirmedMisses--;
					double missEntropy = entropyWithLie(new char[0]);
					numUnconfirmedMisses++;
					
					alphabet[i] = A_MISSED_UNCONFIRMED;

					double totalEntropy = probsLie * lieEntropy + (1.0 - probsLie) * missEntropy;
					if (totalEntropy == Double.POSITIVE_INFINITY) totalEntropy = Double.MAX_VALUE;
					
					if (totalEntropy <= minEntropy) {
						pln("0 under, testing " + indexToChar(i)+ " " + totalEntropy);
						minEntropy = totalEntropy;
						guessInt = i;
					}
				}
			}
		}
		
		// pln("Now guessing unguessed letters\n");

		for (int i = 0; i < ALPH_LENGTH; i++) {

			// pln("wtf\n");

			if (alphabet[i] == A_UNGUESSED) // unguessed
			{
				// Probability that the guess is a hit
				double probsHit = probsOfHit(i);

//				if (probsHit == 1.0) {
//					// printf("prob of hit was 1.0, returning early\n");
//
//					guessInt = i;
//					return guessInt;
//
//				} else 
				if (probsHit > 0) {

					// printf("\tGuessing '%c'\n", indexToChar(i));
					// printf("Prob of hit: %f\n", probsHit);

					// Simulating entropy if the guess was a hit
					char[] neededChars = new char[1];
					neededChars[0] = indexToChar(i);
					alphabet[i] = A_TEMP;

					double hitEntropy;
					if (lieFound)
						hitEntropy = entropyNoLie(neededChars);
					else
						hitEntropy = entropyWithLie(neededChars);
					// printf("Entropy if hit: %f\n", hitEntropy);

					// Simulating entropy if the guess was a miss
					if (lieFound)
						alphabet[i] = A_MISSED_CONFIRMED;
					else {
						alphabet[i] = A_MISSED_UNCONFIRMED;
						numUnconfirmedMisses++;
					}

					double missEntropy;
					if (lieFound)
						missEntropy = entropyNoLie(new char[0]);
					else
						missEntropy = entropyWithLie(new char[0]);

					if (!lieFound)
						numUnconfirmedMisses--;
					// printf("Entropy if miss: %f\n", missEntropy);

					// Total entropy
					double totalEntropy = probsHit * hitEntropy + (1.0 - probsHit) * missEntropy;
					if (totalEntropy == Double.POSITIVE_INFINITY) totalEntropy = Double.MAX_VALUE;
					// printf("Total entropy: %f\n", totalEntropy);

					if (totalEntropy <= minEntropy) {
						pln(String.format("%.2f %.2f %.2f", probsHit, hitEntropy, missEntropy));
						pln("1 under, testing " + indexToChar(i)+ " " + totalEntropy);
						minEntropy = totalEntropy;
						guessInt = i;
					}
					
					alphabet[i] = A_UNGUESSED;
				}
			}
		}

//		pln("Best guess was %c with entropy %f\n", indexToChar(guessInt), minEntropy);
		return guessInt;
	}

	double entropyNoLie(char[] neededChars) {
		double[] e = entropyOfStatus(1.0, neededChars);
		double entropy = 0.0;
		for (int i = 0; i < numWords; i++) {
			entropy += e[i];
		}
		return entropy;
	}

	double entropyWithLie(char[] neededChars) {
		if (numUnconfirmedMisses == 0 || lieFound)
			return entropyNoLie(neededChars);

		double probOfLie = probSomethingIsLie();
		double probOfNotLie = probThereAreNoLies();

		double[] summedEntropies = new double[numWords];

		
		ArrayList<Integer> unconfirmedMisses = new ArrayList<Integer>();
		for(int i=0;i<ALPH_LENGTH;i++) {
			if (alphabet[i] == A_MISSED_UNCONFIRMED) {
				unconfirmedMisses.add(i);
				alphabet[i] = A_MISSED_CONFIRMED;
			}
		}
		numUnconfirmedMisses = 0;

		double[] e = entropyOfStatus(probOfNotLie, neededChars);
		for (int i = 0; i < numWords; i++) {
			if (e[i] == Double.MAX_VALUE)
				summedEntropies[i] = Double.MAX_VALUE;
			else
				summedEntropies[i] += e[i];
		}
		
		numUnconfirmedMisses = unconfirmedMisses.size();
		for(int i=0;i<numUnconfirmedMisses;i++) {
			alphabet[unconfirmedMisses.get(i)] = A_MISSED_UNCONFIRMED;
		}

		char[] neededCharsAppended = new char[neededChars.length + 1];
		if (neededChars.length == 1)
			neededCharsAppended[0] = neededChars[0];

//		 pln("entropy before adding misses: " + summedEntropies[0]);

		for (int i = 0; i < ALPH_LENGTH; i++) {
			if (alphabet[i] == A_MISSED_UNCONFIRMED) {
				
				alphabet[i] = A_TEMP; // set to must have
				neededCharsAppended[neededCharsAppended.length - 1] = indexToChar(i);

				e = entropyOfStatus(probOfLie, neededCharsAppended);
				
				alphabet[i] = A_MISSED_UNCONFIRMED;
				
				for (int j = 0; j < numWords; j++) {
					if (e[j] == Double.MAX_VALUE)
						summedEntropies[j] = Double.MAX_VALUE;
					else
						summedEntropies[j] += e[j];
				}
			}
		}

		double entropy = 0.0;
		for (int i = 0; i < numWords; i++) {
			if (summedEntropies[i] == Double.MAX_VALUE) entropy =  Double.MAX_VALUE;
			else if (entropy != Double.MAX_VALUE) entropy += summedEntropies[i]; 
		}
		return entropy;
	}

	double[] entropyOfStatus(double probFactor, char[] neededChars) {
		getValidWords(neededChars);
		// pln("valid combos: %d\n", validCombos);

		double[] entropies = new double[numWords];
		for (int i = 0; i < numWords; i++) {

			int numSpecificValidWords = specificValidWords[i];
			if (numSpecificValidWords == 0)
				entropies[i] = Double.MAX_VALUE;
			else
				entropies[i] = probFactor * log((double) numSpecificValidWords / probFactor);
		}

		return entropies;
	}

	final double LOG_BASE_2 = Math.log(2);

	double log(double a) {
		return Math.log(a) / LOG_BASE_2;
	}

	void pln(String s) {
		System.out.println(s);
	}

	char indexToChar(int index) {
		char[] chars = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
				't', 'u', 'v', 'w', 'x', 'y', 'z', '\'' };
		return chars[index];
	}

	int charToIndex(char c) {
		switch (c) {
		case 'a':
			return 0;
		case 'b':
			return 1;
		case 'c':
			return 2;
		case 'd':
			return 3;
		case 'e':
			return 4;
		case 'f':
			return 5;
		case 'g':
			return 6;
		case 'h':
			return 7;
		case 'i':
			return 8;
		case 'j':
			return 9;
		case 'k':
			return 10;
		case 'l':
			return 11;
		case 'm':
			return 12;
		case 'n':
			return 13;
		case 'o':
			return 14;
		case 'p':
			return 15;
		case 'q':
			return 16;
		case 'r':
			return 17;
		case 's':
			return 18;
		case 't':
			return 19;
		case 'u':
			return 20;
		case 'v':
			return 21;
		case 'w':
			return 22;
		case 'x':
			return 23;
		case 'y':
			return 24;
		case 'z':
			return 25;
		case '\'':
			return 26; // apostrophe
		}
		pln("Something went wrong");
		return -1;
	}

}
