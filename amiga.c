/* $Id: amiga.c,v 1.3 1998/03/22 22:31:17 drd Exp $ */

/* GNUPLOT - amiga.c */

/*[
 * Copyright 1986 - 1993, 1998   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
]*/

/*
 * amiga.c
 *
 * Written by Carsten Steger <stegerc@informatik.tu-muenchen.de>
 *
 * Popen and pclose have the same semantics as their UNIX counterparts.
 *
 * Additionally, they install an exit trap that closes all open pipes,
 * should the program terminate abnormally.
 */


#include <stdio.h>
#include <ios1.h>
#include <error.h>
#include <string.h>
#include <stdlib.h>

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>

#ifdef PIPES  /* dont bother if pipes are not being used elsewhere */

/* Maximum number of open pipes. If this is set to a number > 10, the code
 * that constructs the pipe names in popen () will have to be modified.
 */
#define MAX_OPEN_PIPES 10

/* We need at least this Dos version to work. */
#define DOS_VERSION 37


/* This data structure is sent to the child process with sm_Cmd set to the
 * command to be executed. When the child is done it sets sm_RetCode to
 * the return code of the executed command.
 */
struct StartupMessage {
  struct Message sm_Msg;
  LONG sm_RetCode;
  UBYTE *sm_Cmd;
};

/* We keep track of the open pipe through this data structure. */
struct PipeFileDescriptor {
  FILE *pfd_File;
  struct StartupMessage pfd_Msg;
};


/* Needed to check for the required Dos version. */
extern struct DosLibrary *DOSBase;

/* This data structure keeps track of the pipes that are still open. */
static struct PipeFileDescriptor OpenPipes[MAX_OPEN_PIPES];

/* The address of the process that calls popen or pclose. */
static struct Process *ThisProcess;

/* Are we called for the first time? */
static LONG FirstCall = TRUE;


/* Prototypes for the functions below. */
FILE *popen (const char *command, const char *mode);
int pclose (FILE *stream);
static void CleanUpPipes (void);
static int __saveds ChildEntry (void);


FILE *popen (command, mode)
const char *command;
const char *mode;
{
  UBYTE PipeName[16];
  ULONG ProcAddress;
  UBYTE HexDigit;
  UBYTE *NextChar;
  struct CommandLineInterface *ThisCli;
  struct PipeFileDescriptor *PipeToUse;
  LONG PipeNumToUse;
  LONG ChildPipeMode;
  BPTR ChildPipe;
  FILE *ParentPipe;
  struct Process *ChildProcess;
  struct TagItem NewProcTags[8] = {
    {NP_Entry, (Tag) ChildEntry},
    {NP_Cli, TRUE},
    {NP_StackSize, 4096},
    {NP_Input, NULL},
    {NP_Output, NULL},
    {NP_CloseInput, FALSE},
    {NP_CloseOutput, FALSE},
    {TAG_DONE, 0}
  };

  /* Test whether we're using the right Dos version. */
  if (DOSBase->dl_lib.lib_Version < DOS_VERSION) {
    errno = EPIPE;
    return NULL;
  }

  /* If we're called for the first time, install exit trap and do some
   * initialisation stuff.
   */
  if (FirstCall) {
    /* Initialise pipe file descriptor table. */
    memset (OpenPipes, 0, sizeof (OpenPipes));
    
    /* Install our exit trap. */
    if (atexit (CleanUpPipes) != 0) {
      errno = EPIPE;
      return NULL;
    }
    FirstCall = FALSE;
  }

  /* If we don't know our process' address yet, we should get it now. */
  if (ThisProcess == NULL)
    ThisProcess = (struct Process *) FindTask (NULL);

  /* Get our Cli structure. */
  ThisCli = Cli ();

  /* Now try to find an empty slot in the pipe file descriptor table.
   * Return NULL if no slot is available.
   */
  for (PipeNumToUse = 0; PipeNumToUse < MAX_OPEN_PIPES; PipeNumToUse++)
    if (OpenPipes[PipeNumToUse].pfd_File == NULL) break;
  if (PipeNumToUse >= MAX_OPEN_PIPES) {
    errno = EMFILE;
    return NULL;
  }
  PipeToUse = &OpenPipes[PipeNumToUse];

  /* Check if the specified mode is valid. */
  if (strcmp (mode, "r") == 0)
    ChildPipeMode = MODE_NEWFILE;
  else if (strcmp (mode, "w") == 0)
    ChildPipeMode = MODE_OLDFILE;
  else {
    errno = EINVAL;
    return NULL;
  }

  /* Make a unique file name for the pipe that we are about to open. The
   * file name has the following format: "PIPE:XXXXXXXX_Y", where
   * XXXXXXXX is the address of our process in hex, Y is the number of the
   * slot in the pipe descriptor table that we will use. The code is
   * equivalent to
   * sprintf (PipeNameWriter, "PIPE:%08lX_%1d", ThisProcess, PipeNumToUse);
   * but it doesn't need sprintf and therefore makes programs that don't
   * use printf a lot shorter.
   */
  strcpy (PipeName, "PIPE:00000000_0");
  NextChar = PipeName + 12;
  ProcAddress = (ULONG) ThisProcess;
  while (ProcAddress != 0) {
    HexDigit = (UBYTE) ProcAddress & 0xf;
    HexDigit = HexDigit < 10 ? HexDigit + '0' : HexDigit - 10 + 'A';
    *NextChar-- = HexDigit;
    ProcAddress >>= 4;
  }
  /* If MAX_OPEN_PIPES > 10, this will have to be modified. */
  PipeName[14] = ((UBYTE) PipeNumToUse) + '0';

  /* Create tags for the child process. */
  if (ThisProcess->pr_CLI)
    NewProcTags[2].ti_Data = ThisCli->cli_DefaultStack << 2;
  else
    NewProcTags[2].ti_Data = ThisProcess->pr_StackSize;

  /* Open both ends of the pipe. The child's side is opened with Open (),
   * while the parent's side is opened with fopen ().
   */
  ChildPipe = Open (PipeName, ChildPipeMode);
  ParentPipe = fopen (PipeName, mode);
  if (ChildPipeMode == MODE_NEWFILE) {
    NewProcTags[3].ti_Data = Input ();
    NewProcTags[4].ti_Data = ChildPipe;
    NewProcTags[5].ti_Data = FALSE;
    NewProcTags[6].ti_Data = TRUE;
  } else {
    NewProcTags[3].ti_Data = ChildPipe;
    NewProcTags[4].ti_Data = Output ();
    NewProcTags[5].ti_Data = TRUE;
    NewProcTags[6].ti_Data = FALSE;
  }
  if (ChildPipe == NULL || ParentPipe == NULL) {
    errno = EPIPE;
    goto cleanup;
  }

  /* Now generate a entry in the pipe file descriptor table. */
  PipeToUse->pfd_Msg.sm_Cmd = malloc (strlen (command) + 1);
  if (PipeToUse->pfd_Msg.sm_Cmd == NULL) {
    errno = ENOMEM;
    goto cleanup;
  }
  strcpy (PipeToUse->pfd_Msg.sm_Cmd, command);
  PipeToUse->pfd_Msg.sm_Msg.mn_ReplyPort = CreateMsgPort ();
  if (PipeToUse->pfd_Msg.sm_Msg.mn_ReplyPort == NULL) {
    errno = ENOMEM;
    goto cleanup;
  }
  PipeToUse->pfd_Msg.sm_Msg.mn_Node.ln_Type = NT_MESSAGE;
  PipeToUse->pfd_Msg.sm_Msg.mn_Node.ln_Pri = 0;
  PipeToUse->pfd_Msg.sm_Msg.mn_Length = sizeof (struct StartupMessage);
  PipeToUse->pfd_File = ParentPipe;

  /* Now create the child process. */
  ChildProcess = CreateNewProc (NewProcTags);
  if (ChildProcess == NULL) {
    errno = ENOMEM;
    goto cleanup;
  }

  /* Pass the startup message to the child process. */
  PutMsg (&ChildProcess->pr_MsgPort, (struct Message *) &PipeToUse->pfd_Msg);

  /* This is the normal exit point for the function. */
  return ParentPipe;

  /* This code is only executed if there was an error. In this case the
   * allocated resources must be freed. The code is actually clearer (at
   * least in my opinion) and more concise by using goto than by using a
   * function (global variables or function parameters needed) or a lot
   * of if-constructions (code gets blown up unnecessarily).
   */
cleanup:
  if (PipeToUse->pfd_Msg.sm_Msg.mn_ReplyPort == NULL)
    DeleteMsgPort (PipeToUse->pfd_Msg.sm_Msg.mn_ReplyPort);
  if (ParentPipe)
    fclose (ParentPipe);
  if (ChildPipe)
    Close (ChildPipe);
  return NULL;
}


int pclose (stream)
FILE *stream;
{
  LONG PipeToClose;

  /* Test whether we're using the right Dos version. */
  if (DOSBase->dl_lib.lib_Version < DOS_VERSION) {
    errno = EPIPE;
    return -1;
  }

  /* Test whether this is the first call to this module or not. If so,
   * pclose has been called before popen and we return with an error
   * because the initialisation has yet to be done.
   */
  if (FirstCall) {
    errno = EBADF;
    return -1;
  }

  /* Search for the correct table entry and close the associated file. */
  for (PipeToClose = 0; PipeToClose < MAX_OPEN_PIPES; PipeToClose++)
    if (OpenPipes[PipeToClose].pfd_File == stream) break;
  if (PipeToClose >= MAX_OPEN_PIPES) {
    errno = EBADF;
    return -1;
  }
  fclose (stream);

  /* Now wait for the child to terminate and get its exit status. */
  WaitPort (OpenPipes[PipeToClose].pfd_Msg.sm_Msg.mn_ReplyPort);
  OpenPipes[PipeToClose].pfd_File = NULL;

  /* Free the allocates resources. */
  DeleteMsgPort (OpenPipes[PipeToClose].pfd_Msg.sm_Msg.mn_ReplyPort);
  free (OpenPipes[PipeToClose].pfd_Msg.sm_Cmd);

  return OpenPipes[PipeToClose].pfd_Msg.sm_RetCode;
}


static void CleanUpPipes ()
{
  LONG Count;
  FILE *Pipe;

  /* Close each open pipe. */
  for (Count = 0; Count < MAX_OPEN_PIPES; Count++) {
    Pipe = OpenPipes[Count].pfd_File;
    if (Pipe != NULL)
      pclose (Pipe);
  }
}


static int __saveds ChildEntry ()
{
  struct Process *ChildProc;
  struct StartupMessage *StartUpMessage;
  LONG ReturnCode;
  struct DosLibrary *DOSBase;
  struct TagItem SysTags[3] = {
    {SYS_Asynch, FALSE},
    {SYS_UserShell, TRUE},
    {TAG_DONE, 0}
  };

  /* We need to open this library, because we don't inherit it from our
   * parent process.
   */
  DOSBase = (struct DosLibrary *) OpenLibrary ("dos.library", DOS_VERSION);

  /* Get the childs process structure. */
  ChildProc = (struct Process *) FindTask (NULL);

  /* Wait for the startup message from the parent. */
  WaitPort (&ChildProc->pr_MsgPort);
  StartUpMessage = (struct StartupMessage *) GetMsg (&ChildProc->pr_MsgPort);

  /* Now run the command and return the result. */
  if (DOSBase != NULL)
    ReturnCode = System (StartUpMessage->sm_Cmd, SysTags);
  else
    ReturnCode = 10000;
  StartUpMessage->sm_RetCode = ReturnCode;

  /* Tell the parent that we are done. */
  ReplyMsg ((struct Message *) StartUpMessage);

  if (DOSBase)
    CloseLibrary ((struct Library *) DOSBase);

  return 0;
}

#endif /* PIPES */
