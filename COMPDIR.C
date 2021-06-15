/*
 *
 * compdir.c
 * Tuesday, 11/1/1994.
 *
 */

#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <GnuMisc.h>
#include <GnuArg.h>
#include <GnuStr.h>

#define MAXFILES 500

typedef struct
   {
   PSZ   pszName;
   ULONG ulSize;
   FDATE fDate;
   FTIME fTime;
   BOOL  bUsed;
   } NFO;
typedef NFO *PNFO;


NFO pnfo1 [MAXFILES];
NFO pnfo2 [MAXFILES];

BOOL bCHANGESONLY;


char szDate [32], szTime[32], szAtt[8];

PSZ DateStr (FDATE fDate)
   {
   sprintf (szDate, "%2.2d-%2.2d-%2.2d", fDate.month, fDate.day, 
                     (fDate.year + 80) % 100);
   return szDate;
   }


PSZ TimeStr (FTIME fTime)
   {
   sprintf (szTime, "%2d:%2.2d%c",
                     (fTime.hours % 12 ? fTime.hours % 12 : 12),
                     fTime.minutes, (fTime.hours > 11 ? 'p' : 'a'));
   return szTime;
   }




void PrintFile (PNFO pnfo)
   {
   if (!pnfo)
      printf ("%37s", " ");
   else
      printf ("%-12s %8lu %s %s", pnfo->pszName, pnfo->ulSize,
               DateStr(pnfo->fDate), TimeStr(pnfo->fTime));
   }



int _cdecl pfnCmpNFO (PNFO pnfo1, PNFO pnfo2)
   {
   if (!pnfo2)
      return 1;

   if (pnfo1->fDate.year  != pnfo2->fDate.year )
      return (int)(pnfo1->fDate.year  - pnfo2->fDate.year );
   if (pnfo1->fDate.month != pnfo2->fDate.month)
      return (int)(pnfo1->fDate.month - pnfo2->fDate.month);
   if (pnfo1->fDate.day   != pnfo2->fDate.day  )
      return (int)(pnfo1->fDate.day   - pnfo2->fDate.day  );

   if (pnfo1->fTime.hours   != pnfo2->fTime.hours  )
      return (int)(pnfo1->fTime.hours   - pnfo2->fTime.hours  );
   if (pnfo1->fTime.minutes != pnfo2->fTime.minutes)
      return (int)(pnfo1->fTime.minutes - pnfo2->fTime.minutes);
   if (pnfo1->fTime.twosecs != pnfo2->fTime.twosecs)
      return (int)(pnfo1->fTime.twosecs - pnfo2->fTime.twosecs);

   return stricmp (pnfo1->pszName, pnfo2->pszName);
   }



PNFO FindMatch (PNFO pnfo, PSZ pszName)
   {
   USHORT i;

   for (i=0; pnfo[i].pszName; i++)
      if (!stricmp (pnfo[i].pszName, pszName))
         return pnfo+i;
   return NULL;
   }



void CompareFiles (PNFO pnfo1, PNFO pnfo2)
   {
   USHORT i;
   PNFO   pnfoMatch;
   int    iCmp;

   printf ("%-37s <=> %-37s\n", ArgGet(NULL, 0), ArgGet(NULL, 1));
   printf ("-------------------------------------------------------------------------------\n");

   for (i=0; pnfo1[i].pszName; i++)
      {
      if (pnfoMatch = FindMatch (pnfo2, pnfo1[i].pszName))
         pnfoMatch->bUsed = TRUE;
         
      iCmp = pfnCmpNFO (pnfo1+i, pnfoMatch);

      if (bCHANGESONLY && !iCmp)
         continue;

      PrintFile (pnfo1 + i);
      if (iCmp > 0)
         printf (" <== ");
      else if (iCmp < 0)
         printf (" ==> ");
      else
         printf ("  =  ");
      PrintFile (pnfoMatch);
      printf ("\n");
      }

   for (i=0; pnfo2[i].pszName; i++)
      {
      if (pnfo2[i].bUsed)
         continue;
      PrintFile (NULL);
      printf (" ==> ");
      PrintFile (pnfo2 + i);
      printf ("\n");
      }
   }




USHORT LoadFiles (PNFO pnfo, PSZ pszPath)
   {
   FILEFINDBUF findbuf;
   USHORT  i, uAtts, uRes, uCt, uIdx = 0;
   HDIR    hdir;
   PSZ     pszMatch;
   BOOL    bMatch;
   char    szCurrDir[256];

   getcwd (szCurrDir, sizeof szCurrDir);
   if (chdir (pszPath))
      Error ("Could not change to path: %s", pszPath);

   uCt   = 1;
   hdir  = HDIR_SYSTEM;
   uAtts = FILE_NORMAL | FILE_ARCHIVED | FILE_SYSTEM | FILE_HIDDEN;

   uRes = DosFindFirst("*.*", &hdir, uAtts, &findbuf, sizeof(findbuf), &uCt, 0L);
   while (!uRes)
      {
      bMatch = TRUE;
      for (i=2; pszMatch = ArgGet (NULL, i); i++)
         if (bMatch = StrMatches (findbuf.achName, pszMatch, FALSE))
            break;

      if (bMatch)
         {
         pnfo[uIdx].pszName = strdup (findbuf.achName);
         pnfo[uIdx].ulSize  = findbuf.cbFile;
         pnfo[uIdx].fDate   = findbuf.fdateLastWrite;
         pnfo[uIdx].fTime   = findbuf.ftimeLastWrite;
         pnfo[uIdx].bUsed   = FALSE;
         pnfo[++uIdx].pszName = NULL; // term marker
         }
      uRes = DosFindNext(hdir, &findbuf, sizeof(findbuf), &uCt);
      }
   DosFindClose (hdir);

   qsort (pnfo, uIdx, sizeof (NFO), pfnCmpNFO);
   chdir (szCurrDir);
   return uIdx;
   }



int _cdecl main (int argc, char *argv[])
   {
   if (ArgBuildBlk ("? *^Changes"))
      Error ("%s", ArgGetErr ());

   if (ArgFillBlk (argv))
      Error ("%s", ArgGetErr ());

   if (argc < 3)
      return printf ("Usage: CompDir [/Changes] path1 path2 filespec...");

   bCHANGESONLY = ArgIs ("Changes");

   LoadFiles (pnfo1, ArgGet (NULL, 0));
   LoadFiles (pnfo2, ArgGet (NULL, 1));
   CompareFiles (pnfo1, pnfo2);

   return 0;
   }

