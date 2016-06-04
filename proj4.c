/**
 * Soubor:  proj4.c
 * Datum:   12.12.2011
 * Autor:   Radim Kubis, xkubis03@stud.fit.vutbr.cz
 * Projekt: Ceske razeni
 * Popis:   Program seradi radky vstupniho souboru podle pravidel razeni
 *          v ceskem jazyce a takto serazene radky zapise do souboru vystupniho.
 *          Pri parametru --usort jsou duplicitni radky na vystupu jen jednou.
 *
 *          Pouziti:
 *
 *              ./proj4 [--loc <kodovani>] <vstup> <vystup> [--usort]
 *
 *          Popis parametru programu:
 *          
 *              --loc <kodovani> - nepovinny parametr programu, timto parametrem
 *                                 lze specifikovat kodovani vstupniho souboru,
 *                                 pokud je rozdilne od nastaveni pracovniho
 *                                 prostredi
 *
 *              <vstup>      - cesta ke vstupnimu souboru, ktery chceme seradit
 *
 *              <vystup>     - cesta k vystupnimu souboru, do ktereho budou
 *                             zapsany serazene radky souboru prvniho
 *
 *              --usort      - nepovinny parametr, rozsireni pro unikatni
 *                             razeni = vystup neobsahuje dva stejne radky
 */

#include <stdio.h>
#include <wchar.h>
#include <errno.h>
#include <stdlib.h>
#include <locale.h>
#include <wctype.h>
#include <string.h>

//Index pismene C
#define C_INDEX 3

// Index pismene H
#define H_INDEX 9

// Index pismene CH
#define CH_INDEX 10

// Velikosti pro alokaci retezce
#define KROK_ZVETSENI 10
#define POCATECNI_VELIKOST 20

// Velikosti pro tabulku symbolu
#define POCET_ZNAKU 42
#define MAX_DELKA_ZNAKU 8

// Hodnoty vysledku funkce pro porovnani
#define ROVNO 0
#define MENSI 1
#define VETSI -1

// Hodnota nenastaveno
#define NONE -2

// Hodnoty pro unikatnost
#define NEUNIKATNI 0
#define UNIKATNI 1

/**
 * Vsechny znaky ocekavane na vstupu podle poradi razeni
 */
wchar_t abeceda[][MAX_DELKA_ZNAKU] = {
  L" ", {L'a', L'\u00E1', L'A', L'\u00C1'}, L"bB", L"cC",
  {L'\u010D', L'\u010C'}, {L'd', L'\u010F', L'D', L'\u010E'},
  {L'e', L'\u00E9', L'\u011B', L'E', L'\u00C9', L'\u011A'},
  L"fF", L"gG", L"hH", L"ch", {L'i', L'\u00ED', L'I', L'\u00CD'},
  L"jJ", L"kK", L"lL", L"mM", {L'n', L'\u0148', L'N', L'\u0147'},
  {L'o', L'\u00F3', L'O', L'\u00D3'}, L"pP", L"qQ", L"rR",
  {L'\u0159', L'\u0158'}, L"sS", {L'\u0161', L'\u0160'},
  {L't', L'\u0165', L'T', L'\u0164'},
  {L'u', L'\u00FA', L'\u016F', L'U', L'\u00DA', L'\u016E'},
  L"vV", L"wW", L"xX", {L'y', L'\u00FD', L'Y', L'\u00DD'}, L"zZ",
  {L'\u017E', L'\u017D'}, L"0", L"1", L"2", L"3", L"4", L"5",
  L"6", L"7", L"8", L"9"
};

/**
 * Kody chyb a stavu programu
 */
enum kody_chyb {
  OK = 0,              // Vse je v poradku
  NENI_PAMET,          // Neni dostatek pameti
  SPATNE_PARAMETRY,    // Spatne zadane parametry
  NEZNAMY_PARAMETR,    // Neznamy parametr
  NASTAVENI_KODOVANI,  // Chyba v nastaveni kodovani
  CTENI_KODOVANI,      // Chyba kodovani pri cteni souboru (1. cast)
  CTENI_KODOVANI2,     // Chyba kodovani pri cteni souboru (2. cast)
  OTEVRENI_SOUBORU,    // Chyba otevirani souboru
  UZAVRENI_SOUBORU,    // Chyba uzavirani souboru
  OSTATNI,             // Neznama chyba
  NAPOVEDA,            // Napoveda
  KONEC                // Zarazka seznamu
};

/**
 * Pole textovych retezcu vypisovanych funkci printErr
 */
const char *RETEZCE[] = {
  /* OK */
  "Vse je v poradku.",
  /* NENI_PAMET */
  "Nepodarilo se alokovat pamet pro beh programu.",
  /* SPATNE_PARAMETRY */
  "Spatne zadane parametry.",
  /* NEZNAMY_PARAMETR */
  "Zadan neznamy parametr: ",
  /* NASTAVENI_KODOVANI */
  "Nepodarilo se nastavit kodovani: ",
  /* CTENI_KODOVANI */
  "Nepodarilo se nacist obsah souboru ",
  /* CTENI_KODOVANI2 */
  " v kodovani ",
  /* OTEVRENI_SOUBORU */
  "Nepodarilo se otevrit soubor: ",
  /* UZAVRENI_SOUBORU */
  "Nepodarilo se uzavrit soubor: ",
  /* OSTATNI */
  "Vyskytla se neznama chyba.",
  /* NAPOVEDA */
  "Program: Ceske razeni\n"
  "Autor:   Radim Kubis, xkubis03@stud.fit.vutbr.cz\n"
  "Popis:   Program seradi radky vstupniho souboru podle pravidel razeni\n"
  "         v ceskem jazyce a takto serazene radky zapise do souboru vystupniho.\n"
  "         Pri parametru --usort jsou duplicitni radky na vystupu jen jednou.\n\n"
  "         Pouziti:\n\n"
  "             ./proj4 [--loc <kodovani>] <vstup> <vystup> [--usort]\n\n"
  "         Popis parametru programu:\n\n"
  "             --loc <kodovani> - nepovinny parametr programu, timto parametrem\n"
  "                                lze specifikovat kodovani vstupniho souboru,\n"
  "                                pokud je rozdilne od nastaveni pracovniho\n"
  "                                prostredi\n\n"
  "             <vstup>      - cesta ke vstupnimu souboru, ktery chceme seradit\n\n"
  "             <vystup>     - cesta k vystupnimu souboru, do ktereho budou\n"
  "                            zapsany serazene radky souboru prvniho\n\n"
  "             --usort      - nepovinny parametr, rozsireni pro unikatni\n"
  "                            razeni = vystup neobsahuje dva stejne radky\n",
  /* KONEC */
  ""
};

/**
 * Struktura pro parametry programu
 */
typedef struct parametry {
  int kod;             // Kod chyby/stavu programu
  char *sada;          // Polozka nastavovaneho kodovani funkci setlocale
  char *chyba;         // Retezec pro ulozeni pomocneho popisu chyby
  int unikatni;        // Polozka pro ulozeni zadaneho parametru --usort
  char *kodovani;      // Kodovani zadane v parametru --loc
  char *souborVstup;   // Retezec se vstupnim souborem
  char *souborVystup;  // Retezec s vystupnim souborem
} PARAMETRY;

/**
 * Struktura pro ulozeni radku
 */
typedef struct radek {
  int delka;       // Delka retezce v jednotlivych znacich       
  int velikost;    // Alokovana velikost
  int pocetZnaku;  // Pocet znaku retezece (kvuli 'ch'), bez mezer
  wchar_t *obsah;  // Retezec znaku
} RADEK;

/**
 * Struktura pro uzel stromu
 */
typedef struct uzel {
  RADEK *retezec;      // Retezec v uzlu stromu
  struct uzel *levy;   // Mensi potomek uzlu ve stromu
  struct uzel *pravy;  // Vetsi potomek uzlu ve stromu
} UZEL;

/**
 * Funkce pro zjisteni zadanych parametru zadanych na prikazovem radku
 * @param argc - pocet vstupnich parametru
 * @param argv - retezce se vstupnimi parametry
 * @return - funkce vraci strukturu se zadanymi parametry
 */
PARAMETRY zjistiParametry(int argc, char *argv[]) {

  // Inicializace struktury
  PARAMETRY par = {
    .kod = OK,     // Pocatecni kod programu je OK
    .sada = NULL,  // Nebyla zatim volana funkce setlocale
	  .chyba = "",   // Pomocny popis chyby je prozatim prazdny
	  .unikatni = NEUNIKATNI, // Unikatnost retezce nebyla zadana
    .kodovani = "",        // Vstupni kodovani zatim nebylo nastaveno
	  .souborVstup = NULL,   // Vstupni soubor nebyl nastaveni
	  .souborVystup = NULL,  // Vystupni soubor nebyl nastaven
  };

  if(argc < 3 || argc > 6) {  // Pokud nebyl zadan spravny pocet parametru
    par.kod = SPATNE_PARAMETRY;  // Chyba parametru
  } else if(argc == 3) {  // Pokud byl pocet parametru 3
    par.souborVstup = argv[1];  // Prvni je vstupni soubor
	  par.souborVystup = argv[2]; // Druhy je vystupni soubor
  } else if(argc == 4) {  // Pokud byl pocet parametru 4
    if(strcmp(argv[3], "--usort") == ROVNO) {  // Musi byt posledni --usort
	    par.unikatni = 1;  // Nastavim unikatnost retezcu
	    par.souborVstup = argv[1];  // Nastavim vstupni soubor
	    par.souborVystup = argv[2]; // Nastavim vystupni soubor
	  } else {  // Pokud nebyl posledni parametr --usort
	    par.chyba = argv[3];  // Chyba v poslednim parametru
	    par.kod = NEZNAMY_PARAMETR;  // Nastavim priznak chyby
	  }
  } else if(argc == 5) {  // Pokud byl pocet parametru 5
    if(strcmp(argv[1], "--loc") == ROVNO) {  // Prvni musi byt --loc
	    par.kodovani = argv[2];  // Nastavim pozadovane kodovani vstupniho souboru
	    par.souborVstup = argv[3];  // Nastavim vstupni soubor
	    par.souborVystup = argv[4]; // Nastavim vystupni soubor
	  } else {  // Pokud nebyl prvni parametr --loc
	    par.chyba = argv[1];  // Nastavim chybny parametr
	    par.kod = NEZNAMY_PARAMETR;  // Nastavim priznak chyby
    }
  } else if(argc == 6) {  // Pokud byl pocet parametru 6
    if(strcmp(argv[1], "--loc") == ROVNO) {  // Prvni musi byt --loc
	    if(strcmp(argv[5], "--usort") == ROVNO) {  // Posledni musi byt --usort
	      par.unikatni = 1;  // Nastavim priznak unikatnosti
		    par.kodovani = argv[2];  // Nastavim pozadovane kodovani souboru
		    par.souborVstup = argv[3];  // Nastavim vstupni soubor
	      par.souborVystup = argv[4]; // Nastavim vystupni soubor
	    } else {  // Pokud nebyl posledni parametr --usort
	      par.chyba = argv[5];  // Nastavim chybny parametr
	      par.kod = NEZNAMY_PARAMETR;  // Nastavim priznak chyby
	    }
	  } else {  // Pokud nebyl prvni parametr --loc
	    par.chyba = argv[1];  // Nastavim chybny parametr
	    par.kod = NEZNAMY_PARAMETR;  // Nastavim priznak chyby
    }
  }

  return par;  // Vracim nactene parametry
}

/**
 * Funkce tiskne chybu, ktera nastala behem programu
 * @param p - parametry vstupu a chyby/stavu
 * Nema navratovou hodnotu
 */
void tiskChyby(PARAMETRY p) {
  if(p.kod < OK || p.kod >= KONEC) {  // Pokud je kod chyby neznamy
    p.kod = OSTATNI;  // Nastavim hodnotu OSTATNI
  }
  
  if(p.kod <= OSTATNI) {  // Pokud se jedna o chybu
    if(p.kod == CTENI_KODOVANI) {  // Kdyz je chyba v kodovani cteni
      // Tisk popisu chyby
      fprintf(stderr, "CHYBA: %s%s%s%s.\n", RETEZCE[p.kod], p.souborVstup,
              RETEZCE[p.kod+1], p.sada);
    } else {  // Pokud se nejedna o chybu v kodovani cteni
      fprintf(stderr, "CHYBA: %s%s\n", RETEZCE[p.kod], p.chyba);  // Tisk chyby
      // Pokud se jedna o chybu prikazove radky
      if(p.kod == SPATNE_PARAMETRY || p.kod == NEZNAMY_PARAMETR) {
        fprintf(stdout, "%s\n", RETEZCE[NAPOVEDA]);  // Tisknu i napovedu
      }
    }
  }
}

/**
 * Funkce pridava znak do retezce radku
 * @param znak - pridavany znak do retezce radku
 * @param r    - retezec pro pridani znaku
 * @return - funkce vraci chybu/stav programu 
 */
int pridejZnak(wchar_t znak, RADEK *r) {
  if(r != NULL) {  // Pokud r neni NULL, pridavam do nej znak
    if(r->obsah == NULL) {  // Pokud je obsah NULL, alokuji pro nej pamet
      r->obsah = (wchar_t*)malloc(POCATECNI_VELIKOST * sizeof(wchar_t));

      if(r->obsah == NULL) {  // Pokud se alokace nezdari,
        return NENI_PAMET;    // vracim odpovidajici chybu
      }

      // Zinicializuji radek r
      r->velikost = POCATECNI_VELIKOST;  // Alokovana velikost retezce je 20
      r->delka = 0;       // Delka retezce je prozatim 0
      r->pocetZnaku = 0;  // Pocet znaku je take 0
    }

    if(r->delka == (r->velikost - 1)) {  // Pokud delka retezce nestaci
      wchar_t *ukazatel;  // Pomocny ukazatel, kdyby nastala chyba

      r->velikost += KROK_ZVETSENI;  // Nastavim novou alokovanou velikost
      // Realokuji misto v pameti
      ukazatel = (wchar_t*)realloc(r->obsah, r->velikost * sizeof(wchar_t));

      if(ukazatel != NULL) {  // Pokud se realokace pameti podarila,
        r->obsah = ukazatel;  // nastavim na ni ukazatel
      } else {  // Pokud se realokace nepodari
        r->velikost -= KROK_ZVETSENI;  // Snizim alokovanou velikost
        return NENI_PAMET;  // Vracim chybu pridelovani pameti
      }
    }
    
    if(!(  // Negovana podminka:
         (r->delka > 0) &&  // Pokud mam v retezci alespon jeden znak
         // Posledni vlozeny je 'c/C'
         (r->obsah[r->delka-1] == L'c' || r->obsah[r->delka-1] == L'C') &&
         (znak == L'h' || znak == L'H')  // a prave vkladany je 'h/H'
         // => obsahuje '[cC][hH]'
        ) &&  // Konec negovane podminky
        znak != L' '  // A pokud vkladany znak neni mezera
      ) {
      r->pocetZnaku++;  // Zvysim pocet znaku retezce o 1
    }

    r->obsah[r->delka] = znak;  // Ulozim vkladany znak
    r->delka++;  // Zvysim delku retezce o 1
  }
  
  return OK;  // Vse probehlo v poradku
}

/**
 * Funkce vypisuje retezec ve strukture RADEK
 * @param vystup - vystupni proud funkce, do ktereho zapisujeme
 * @param r      - retezec pro vypis
 * Nema navratovou hodnotu 
 */
void vypisRadek(FILE *vystup, RADEK *r) {
  if(r != NULL) {  // Pokud r neni NULL
    for(int i = 0; i < r->delka; i++) {  // Prochazim v cyklu jeho znaky
      fputwc(r->obsah[i], vystup);  // Vypisuji kazdy znak do proudu
    }
    fputwc(L'\n', vystup);  // Vypis retezce zakoncim koncem radku
  }
}

/**
 * Funkce nacita jeden radek
 * @param vstup - vstupni proud, ze ktereho cteme znaky
 * @param r     - retezec pro ulozeni radku
 * @return - vraci kod chyby/stavu programu
 */
int nactiRadek(FILE *vstup, RADEK *r) {
  wint_t znak;  // Promenna pro znak, ktery prave nacitam

  while(1) {  // Nekonecny cyklus pro nacitani znaku z proudu
    znak = fgetwc(vstup);  // Nactu jeden znak z proudu

    if(znak == WEOF) {  // Pokud jsem na konci souboru
      // Vracim WEOF v pripade konce souboru bez nacteni znaku, nebo OK
      return (r->delka == 0) ? znak : OK;
    } else if (znak == L'\n') {  // Pokud je znak konce radku
      return OK;  // Vracim OK - konec retezce na radku
    } else {  // Pokud neni znak WEOF ani konec radku,
      pridejZnak(znak, r);  // ulozim ho
    }
  }
}

/**
 * Funkce uvolnuje pamet po strukture RADEK
 * @param r - retezec, ktery chceme uvolnit
 * Nema navratovou hodnotu
 */
void uvolniRadek(RADEK *r) {
  if(r != NULL) {  // Pokud r neni NULL
    if(r->obsah != NULL) {  // Pokud jeho obsah neni NULL
      free(r->obsah);   // Uvolnim misto po jeho obsahu
      r->obsah = NULL;  // Nastavim ukazatel na NULL
    }
    free(r);   // Uvolnim misto po radku
    r = NULL;  // Nastavim ukazatel na NULL
  }
}

/**
 * Funkce alokuje pamet pro strukturu RADEK
 * Funkce nema parametry
 * @return - vraci ukazatel na misto v pameti pro radek, nebo NULL 
 */
RADEK *init() {
  RADEK *pomocny = NULL;  // Pomocny ukazatel na misto v pameti
  
  // Zkusim alokovat pamet pro radek
  if((pomocny = (RADEK*)malloc(sizeof(RADEK))) == NULL) {
    return NULL;  // Pokud se nezdari, vracim NULL
  }

  // Pokud se podari pamet alokovat
  pomocny->obsah = NULL;  // Zinicializuji obsah radku na NULL,
  pomocny->velikost = 0;  // alokovanou velikost na 0,
  pomocny->delka = 0;     // delku retezce na 0
  pomocny->pocetZnaku = 0;// a pocet znaku take na 0
  
  return pomocny;  // Vracim ukazatel na misto v pameti
}

/**
 * Funkce zjistuje poradi znaku v razeni
 * @param znak - znak, ktereho poradi chceme zjistit
 * @return - funkce vraci poradi pro razeni 
 */
int dejPoradi(wchar_t znak) {
  int index = 0;  // Pocatecni index v poli znaku

  for(; index < POCET_ZNAKU; index++) {  // Hledam znak v poradi razeni
    if(wcschr(abeceda[index], znak) != NULL) {  // Pokud je znak v poradi razeni
      break;  // Koncim cyklus, poradi je ulozeno v indexu
    }
  }

  // Pokud znak nebyl v tabulce nalezen, je jeho poradi az za tabulkou
  // a pozici urcuje take index

  return index;  // Vracim poradi znaku
}

/**
 * Funkce zjistuje pozici znaku v jeho variantach
 * @param znak   - znak, ktery hledame
 * @param poradi - poradi, ve kterem hledame
 * @param unikatni     - pokud rozlisujeme velikost pismen pro unikatni retezec 
 */
int dejPozici(wchar_t znak, int poradi, int unikatni) {
  unsigned int index = 0;  // Prozatim je index na 0

  if(unikatni != UNIKATNI) {  // Pokud nechci rozlisovat velikost pismen
    znak = towlower(znak);  // Zmenim znak na maly
  }
  
  // Hledam znak v zadanem poradi jeho variant
  for(; index < wcslen(abeceda[poradi]); index++) {
    if(znak == abeceda[poradi][index]) {  // Pokud jsem znak nasel
      break;  // Koncim cyklus - index urcuje pozici
    }
  }
  
  return index;  // Vracim pozici znaku
}

/**
 * Funkce porovnava dva retezce pro poradi v razeni
 * @param r1 - prvni retezec pro porovnani
 * @param r2 - druhy retezec pro porovnani
 * @param unikatni - jestli budeme rozlisovat velikost pismen
 * @return - vraci 0 pro stejne retezce,
 *           vraci 1 pro mensi retezec r1,
 *           vraci -1 pro mensi retezec r2   
 */
int porovnejRadky(RADEK *r1, RADEK *r2, int unikatni) {
  int poradi1 = 0;  // Poradi aktualne proverovaneho znaku v razeni retezce r1
  int poradi2 = 0;  // Poradi aktualne proverovaneho znaku v razeni retezce r2
  int pozice1 = NONE;  // Pozice stejneho znaku v r1
  int pozice2 = NONE;  // Pozice stejneho znaku v r2
  int vysledek = 0; // Vysledna hodnota vracena funkci
  int mensiZnak = NONE;   // Vysledek porovnani na aktualnim znaku
  int mensiRadek = NONE;  // Vysledek porovnani na variante aktualniho znaku

  for(int indexR1 = 0, indexR2 = 0;  // Pocatecni indexy retezcu jsou 0
      // Porovnavam, dokud nejsem na konci nektereho z retezcu
      indexR1 < r1->delka && indexR2 < r2->delka;
      indexR1++, indexR2++  // Zvysuji index v retezcich o 1
     ) {

    if(r1->obsah[indexR1] == L' ') {  // Pokud je na aktualni pozici r1 mezera
      // Dokud je index mensi nez delka a nasledujici znak je mezera
      while(((indexR1 + 1) < r1->delka) && (r1->obsah[indexR1 + 1] == L' ')) {
        indexR1++;  // Zvysuji index r1
      }
    }
    
    if(r2->obsah[indexR2] == L' ') {  // Pokud je na aktualni pozici r2 mezera
      // Dokud je index mensi nez delka a nasledujici znak je mezera
      while(((indexR2 + 1)< r2->delka) && (r2->obsah[indexR2 + 1] == L' ')) {
        indexR2++;  // Zvysuji index r2
      }
    }

	  poradi1 = dejPoradi(r1->obsah[indexR1]);  // Zjistim poradi pismene v r1
    // Zjistim pozici znaku (pro unikatnost)
    pozice1 = dejPozici(r1->obsah[indexR1], poradi1, unikatni);
    // Aktualni znak r1 je 'c/C'
    if(poradi1 == C_INDEX) {
	    if((indexR1 + 1) < r1->delka) {  // Pokud nejsem na konci retezce r1
        // a nasledujici znak r1 je 'h/H'
        if((r1->obsah[indexR1+1] == L'h') || (r1->obsah[indexR1+1] == L'H')) {
          poradi1 = CH_INDEX;  // Nastavim poradi v razeni na poradi CH
          indexR1++;  // Zvysim index retezce r1 o 1
        }
	    }
	  }

	  poradi2 = dejPoradi(r2->obsah[indexR2]);  // Zjistim poradi pismene v r2
    // Zjistim pozici znaku (pro unikatnost)
    pozice2 = dejPozici(r2->obsah[indexR2], poradi2, unikatni);
    // Aktualni znak r2 je 'c/C'
    if(poradi2 == C_INDEX) {
	    if((indexR2 + 1) < r2->delka) {  // Pokud nejsem na konci retezce r2
        // a nasleduji znak r2 je 'h/H'
        if((r2->obsah[indexR2+1] == L'h') || (r2->obsah[indexR2+1] == L'H')) {
          poradi2 = CH_INDEX;  // Nastavim poradi v razeni na poradi CH
          indexR2++;  // Zvysim index retezce r2 o 1
        }
	    }
	  }
	
    if(unikatni != UNIKATNI &&  // Pokud neni zadana unikatnost retezcu,
	     poradi1 == poradi2 &&  // zaroven jsou pismena na stejnem poradi
       // a zaroven jsou pismena na stejne pozici
	     dejPozici(r1->obsah[indexR1], poradi1, NEUNIKATNI) ==
       dejPozici(r2->obsah[indexR2], poradi2, NEUNIKATNI)
	    ) {
      continue;  // Pokracuji na dalsi pismeno v retezcich
    // Pokud zalezi na velikosti pismen a pismena jsou stejna,
    } else if(unikatni == UNIKATNI &&
              pozice1 == pozice2 &&
              r1->obsah[indexR1] == r2->obsah[indexR2]
             ) {
      continue;  // pokracuji dal
    } else {  // Kdyz se pismena lisi
      if(poradi1 != poradi2) {  // Pokud se poradi pismen lisi
        mensiRadek = (poradi1 < poradi2) ? MENSI : VETSI;  // Nastavim poradi
        break;  // Cyklus porovnani konci
      // Pokud je poradi pismen stejne a jeste se nelisily na variante
      } else if(mensiZnak == NONE) {
        // Pokud je index CH_INDEX a pozice jsou stejne, nebo se nejedna o CH
        if((poradi1 == CH_INDEX && pozice1 == pozice2) || poradi1 != CH_INDEX) {
			    // Zjistim pozici varianty pismene r1
			    pozice1 = dejPozici(r1->obsah[indexR1], poradi1, unikatni);

			    // Zjistim pozici varianty pismene r2
			    pozice2 = dejPozici(r2->obsah[indexR2], poradi2, unikatni);
		    }
        
        // Ulozim si mensi pozici varianty pismene
        mensiZnak = (pozice1 < pozice2) ? MENSI : VETSI;
      }
    }
  }

  if(mensiRadek == NONE) {  // Stejne retezce, bez uvahy varianty pismene
	  if(r1->delka == r2->delka) {  // Pokud se nelisi v delce
      // Jsou absolutne stejne, nebo se lisi na znaku
	    vysledek = (mensiZnak == NONE) ? ROVNO : mensiZnak;
	  } else {  // Pokud nejsou stejne dlouhe
	    // Bude poradi podle delky retezcu
	    vysledek = (r1->pocetZnaku < r2->pocetZnaku) ? MENSI : VETSI;
	  }
  } else {
    // Pro ruzne retezce na znaku bude vysledek podle poradi znaku
    vysledek = mensiRadek;
  }
  
  return vysledek;  // Vracim vysledek
}

/**
 * Funkce uvolni misto po uzlu stromu
 * @param uzel - uzel pro uvolneni pameti
 * Nema navratovou hodnotu 
 */
void uvolniUzel(UZEL *uzel) {
  if(uzel != NULL) {  // Pokud neni uzel NULL
    if(uzel->retezec != NULL) {  // Pokud neni obsah retezce NULL
      uvolniRadek(uzel->retezec);  // Uvolnim jeho retezec
    }

    free(uzel);  // a uvolnim po nem misto
    uzel = NULL;  // Ukazatel nastavim na NULL
  }
}

/**
 * Funkce uvolni misto po stromu
 * @param strom - strom pro uvolneni
 * Nema navratovou hodnotu 
 */
void _uvolniStrom(UZEL *strom) {
  if(strom->levy != NULL) {  // Pokud ma aktualni uzel stromu leveho potomka
    _uvolniStrom(strom->levy);  // Zanorim se vlevo pro uvolneni podstromu
  }

  if(strom->levy != NULL) {  // Az se vynorim z leveho podstromu
    uvolniUzel(strom->levy);  // Uvolnim levy uzel
  }

  if(strom->pravy != NULL) {  // Pokud ma aktualni uzel stromu praveho potomka
    _uvolniStrom(strom->pravy);  // Zanorim se vpravo pro uvolneni podstromu
  }

  if(strom->pravy != NULL) {  // Az se vynorim z praveho podstromu
    uvolniUzel(strom->pravy);  // Uvolnim pravy uzel
  }
}

/**
 * Obalovaci funkce pro uvolneni stromu, musi uvolnit i uzel stromu
 * @param strom - strom, ktery chci uvolnit
 * Nema navratovou hodnotu
 */
void uvolniStrom(UZEL *strom) {
  if(strom != NULL) {  // Pokud strom neni NULL
    _uvolniStrom(strom);  // Uvolnim cely strom
    uvolniUzel(strom);  // Uvolnim uzel stromu
    strom = NULL;  // Nastavim strom na NULL
  }
}

/**
 * Funkce pro pridani retezce do stromu
 * @param strom - strom pro pridani retezce
 * @param r     - retezec, ktery pridavam
 * @param unikatni - jestli bude zalezet na velikosti pismen  
 * Nema navratovou hodnotu 
 */
void pridejUzel(UZEL *strom, RADEK *r, int unikatni) {

  if(strom != NULL) {  // Pokud strom neni NULL
    if(strom->retezec == NULL) {  // Pokud je strom prazdny,
      strom->retezec = r;  // pridam retezec jako koren stromu
    } else {  // Pokud koren neni prazdny
      int porovnani = ROVNO;  // Promenna pro vysledek porovnani dvou retezcu
      UZEL *aktualni = strom;  // Ukazatel na aktualni podstrom pro prochazeni
      UZEL *novy = (UZEL*)malloc(sizeof(UZEL));  // Alokace noveho uzlu
      novy->retezec = r;   // Retezec v novem uzlu bude r
      novy->levy = NULL;   // Levy potomek neexistuje
      novy->pravy = NULL;  // Pravy potomek neexistuje
      
      while(1) {  // Dokud jsem nenasel pozici pro retezec ve stromu
        // Porovnam retezec v aktualnim uzlu a vkladany
        porovnani = porovnejRadky(aktualni->retezec, r, unikatni);

        // Kdyz jsou retezce stejne a chci unikatni retezce
		    if(porovnani == ROVNO && unikatni == UNIKATNI) {
          uvolniUzel(novy);  // Uvolnim misto po vkladanem uzlu
          break;  // Cyklus konci
        // Pokud jsou retezce stejne nebo je prvni mensi
        } else if(porovnani >= ROVNO) {
          if(aktualni->pravy == NULL) {  // Pokud je pravy potomek prazdny
            aktualni->pravy = novy;  // Vlozim stejny/vetsi retezec vpravo
            break;  // Cyklus konci
          } else {  // Pokud neni pravy potomek volny
            aktualni = aktualni->pravy;  // Nastavim aktualni uzel na pravy
          }
        } else if (porovnani == VETSI) {  // Pokud je prvni retezec vetsi
          if(aktualni->levy == NULL) {  // Pokud je levy potomek prazdny
            aktualni->levy = novy;  // Vlozim mensi retezec vlevo
            break;  // Cyklus konci
          } else {  // Pokud neni levy potomek prazdny
            aktualni = aktualni->levy;  // Nastavim aktualni uzel na levy
          }
        }
      }
    }
  }
}

/**
 * Funkce pro vypis stromu od nejmensi prvku po nejvetsi
 * @param strom  - strom pro vypis
 * @param vystup - vystupni proud pro vypis 
 * Nema navratovou hodnotu 
 */
void vypisStrom(UZEL *strom, FILE *vystup) {

  if(strom->levy != NULL) {  // Dokud existuje levy potomek
    vypisStrom(strom->levy, vystup);  // Zanorim se vlevo do stromu
  }

  // Az se maximalne zanorim vlevo
  vypisRadek(vystup, strom->retezec);  // Vypisi retezec v uzlu

  if(strom->pravy != NULL) {  // Dokud existuje pravy potomek
    vypisStrom(strom->pravy, vystup);  // Zanorim se vpravo
  }
}

/**
 * Hlavni funkce main programu
 * @param argc - pocet vstupnich parametru programu
 * @param argv - retezce vstupnich parametru programu
 * @return - pokud pri behu programu nenastala chyba,
 *           vraci hodnotu EXIT_SUCCESS,
 *           kdyz nastane chyba, vraci hodnotu EXIT_FAILURE
 */
int main(int argc, char* argv[]) {
  RADEK *r = NULL;      // Pomocna promenna pro vkladany radek
  UZEL *strom = NULL;   // Uzel pro reprezentaci stromu retezcu
  FILE *vstup = NULL;   // Ukazatel na vstupni soubor
  FILE *vystup = NULL;  // Ukazatel na vystupni soubor

  PARAMETRY p = zjistiParametry(argc, argv);  // Zjistim si parametry programu
  
  if(p.kod != OK) {  // Pokud se nepodarilo bez chyby zjistit parametry
    tiskChyby(p);  // Tisknu chybu programu
	  return EXIT_FAILURE;  // a program konci
  }
  
  // Pokud se nepodari nastavit kodovani
  if((p.sada = setlocale(LC_CTYPE, p.kodovani)) == NULL) {
    p.chyba = p.kodovani;  // Ulozim si kodovani, ktere zpusobilo chybu
	  p.kod = NASTAVENI_KODOVANI;  // Nastavim priznak chyby
  // Pokud se nepodari alokovat misto pro koren stromu
  } else if((strom = (UZEL*)malloc(sizeof(UZEL))) == NULL) {
    p.kod = NENI_PAMET;  // Nastavim priznak chyby
  } else {  // Pokud bylo dosud vse bez chyby, zinicializuji koren stromu
    strom->retezec = NULL;
    strom->levy = NULL;
    strom->pravy = NULL;
  }

  if(p.kod != OK) {  // Pokud se vyskytla v programu chyba
    tiskChyby(p);  // Vytisknu jeji popis
    return EXIT_FAILURE;  // Program konci
  }
  
  // Pokud se nepodari otevrit vstupni soubor
  if((vstup = fopen(p.souborVstup, "r")) == NULL) {
    p.chyba = p.souborVstup;   // Ulozim si jeho jmeno
    p.kod = OTEVRENI_SOUBORU;  // Nastavim priznak chyby
  // Pokud se soubor otevrit podari
  } else {
    while(1) {  // V cyklu nacitam znaky radky ze souboru
      if((r = init()) == NULL) {  // Pokud se nepodari alokovat misto pro radek
	      p.kod = NENI_PAMET;  // Nastavim priznak chyby
        break;  // Cyklus konci
      }
      
      if(nactiRadek(vstup, r) != OK) {  // Pokud se nepodari nacist radek
        if(errno == EILSEQ) {  // Kdyz je chyba v kodovani znaku v souboru
          p.kod = CTENI_KODOVANI;  // Nastavim tento priznak
        }
		    uvolniRadek(r);  // Uvolnim misto po radku
        break;  // Cyklus konci
	    }

      pridejUzel(strom, r, p.unikatni);  // Pokud mam nacteny radek, pridam
	  }
  }

  if(vstup != NULL && fclose(vstup) == EOF) {  // Pokud se nepodari uzavirani vstupniho souboru
    p.kod = UZAVRENI_SOUBORU;  // Nastavim priznak chyby
    p.chyba = p.souborVstup;   // Ulozim si nazev souboru
  }

  if(p.kod != OK) {  // Pokud kod chyby/stavu programu neni OK
    tiskChyby(p);  // Tisknu popis chyby
    uvolniStrom(strom);  // Uvolnim misto po stromu
    return EXIT_FAILURE;  // Program konci
  }

  // Pokud se nepodari otevirani vystupniho souboru
  if((vystup = fopen(p.souborVystup, "w")) == NULL) {
    p.chyba = p.souborVystup;  // Ulozim si nazev souboru
    p.kod = OTEVRENI_SOUBORU;  // Nastavim priznak chyby
  } else {  // Kdyz se soubor podari otevrit
    vypisStrom(strom, vystup);  // Vypisu serazene radky do souboru
    if(fclose(vystup) == EOF) {  // Kdyz se nepodari uzavirani souboru
      p.kod = UZAVRENI_SOUBORU;  // Nastavim priznak chyby
      p.chyba = p.souborVystup;  // Ulozim si nazev souboru
    }
  }

  uvolniStrom(strom);  // Uvolnim misto po stromu

  if(p.kod != OK) {  // Pokud neprobehl program v poradku
    tiskChyby(p);  // Tisknu chybu programu
    return EXIT_FAILURE;  // Program konci
  }

  return EXIT_SUCCESS;  // Uspesny konec programu
}
