;;;; doc2texi.el -- generate a texinfo file from the gnuplot doc file

;; Copyright (C) 1999 Bruce Ravel

;; Author:     Bruce Ravel <ravel@phys.washington.edu>
;; Maintainer: Bruce Ravel <ravel@phys.washington.edu>
;; Created:    March 23 1999
;; Updated:
;; Version:    $Id:$
;; Keywords:   gnuplot, document, info

;; This file is not part of GNU Emacs.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This lisp script is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
;;
;; Permission is granted to distribute copies of this lisp script
;; provided the copyright notice and this permission are preserved in
;; all copies.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; send bug reports to the authors (ravel@phys.washington.edu)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Commentary:
;;
;; I suppose the most immediate question to ask is "Why do this in
;; emacs lisp???"  While it is true that the gnuplot.doc file lends
;; itself to processing by a 1 pass filter, there are some aspects of
;; texinfo files that are a bit tricky and would require 2 or perhaps
;; three passes.  Specifically, getting all of the cross references
;; made correctly is a lot of work.  Fortunately, texinfo-mode has
;; functions for building menus and updating nodes.  This saves a lot
;; of sweat and is the principle reason why I decided to write this is
;; emacs lisp.
;;
;; Everything else in gnuplot is written in C for the sake of
;; portability.  Emacs lisp, of course, requires that you have emacs.
;; For many gnuplot users, that is not a good assumption.  However,
;; the likelihood of needing info files in the absence of emacs is
;; very, very slim.  I think it is safe to say that someone who needs
;; info files has emacs installed and thus will be able to use this
;; program.
;;
;; Since this is emacs, I am not treating the gnuplot.doc file as a
;; text stream.  It seems much more efficient in this context to treat
;; it as a buffer.  All of the work is done by the function
;; `d2t-doc-to-texi'.  Each of the conversion chores is handled by an
;; individual function.  Each of thse functions is very similar in
;; structure.  They start at the top of the buffer, search forward for
;; a line matching the text element being converted, perform the
;; replacement in place, and move on until the end of the buffer.
;; These text manipulations are actually quite speedy.  The slow part
;; of the process is using the texinfo-mode function to update the
;; nodes and menus.  Still, I think this approach compares well with
;; the time cost of compiling and running a C program.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Use:
;;
;; I intend that this be used from the command line or from a Makefile
;; like so:
;;
;;      emacs -batch -l doc2texi.el -f d2t-doc-to-texi
;;
;; This will start emacs in batch mode, load this file, run the
;; converter, then quit.  This takes about 30 seconds my 133 MHz
;; Pentium.  It also sends a large number of mesages to stderr, do you
;; may want to redirect stderr to /dev/null or to a file.
;;
;; Then you can do
;;
;;      makeinfo gnuplot.info
;;
;; You may want to use the --no-split option.
;;
;; The converter makes some serious assumptions about where files are
;; located.  I am assuming that this will be used as a part of a
;; normal gnuplot installation.  Thus it assumes that the file
;; "gnuplot.doc" is in the current working directory, and that the
;; various .trm files are in "../term/".  Walking the directories is
;; done using Emacs' platform non-specific functions.  If these files
;; are not found, an error will be signaled and the converter will
;; quit.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; History:
;;
;;  0.1  Mar 23 1999 <BR> Initial version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Acknowledgements:
;;
;; Lars Hecking asked me to look into the doc -> info problem.  Silly
;; me, I said "ok."  This is what I came up with.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; To do:
;;
;;  1. internal cross-references: these appear as `blah blah`.  need
;;     to watch out for "`blah blah`" which appear sometimes.  also
;;     need to pluck off last word in between `` for use with
;;     @ref{blah}
;;  2. html anchors <a>..</a> are currently commented out.  these need
;;     to be fixed up
;;  3. save the file to gnuplot.info, overwriting the old one
;;  4. catch errors gracefully, particularly when looking for files.
;;  5. fetch terminal information (can configure tell me which ones to
;;     use? or should I use them all?)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Code:

(require 'cl)

(defconst d2t-work-buffer-name "*doc2texi*"
  "Name of scratch buffer where the doc file will be converted into a
  texi file.")
(defconst d2t-doc-file-name "gnuplot.doc"
  "Name of the gnuplot.doc file.")

(defconst d2t-texi-header
  "\\input texinfo   @c -*-texinfo-*-

@c %**start of header
@setfilename gnuplot.info
@settitle Gnuplot: An Interactive Plotting Program
@setchapternewpage odd
@c %**end of header

@c define the command and options indeces
@defindex cm
@defindex op

@direntry
* GNUPLOT: (gnuplot).             An Interactive Plotting Program
@end direntry

@ifnottex
@node Top, Introduction, (dir), (dir)
@top Master Menu
@end ifnottex

@example
                       GNUPLOT

            An Interactive Plotting Program
             Thomas Williams & Colin Kelley
          Version 3.7 organized by: David Denholm

 Copyright (C) 1986 - 1993, 1998   Thomas Williams, Colin Kelley

       Mailing list for comments: info-gnuplot@@dartmouth.edu
     Mailing list for bug reports: bug-gnuplot@@dartmouth.edu

         This manual was prepared by Dick Crawford
                   3 December 1998


Major contributors (alphabetic order):
@end example

"
  "Texinfo header.")

(defconst d2t-main-menu
  "@menu
@end menu"
  "Main menu.")

(defconst d2t-texi-footer
  "@node Concept_Index
@unnumbered Concept Index
@printindex cp

@node Command_Index
@unnumbered Command Index
@printindex cm

@node Options_Index
@unnumbered Options Index
@printindex op

@node Function_Index
@unnumbered Function Index
@printindex fn

@c @shortcontents
@contents
@bye
"
  "Texinfo file terminator.")

(defvar d2t-level-1-alist nil
  "Alist of level 1 tags and markers.")
(defvar d2t-commands-alist nil
  "Alist of commands and markers.")
(defvar d2t-set-show-alist nil
  "Alist of options and markers.")
(defvar d2t-functions-alist nil
  "Alist of functions and markers.")
(defvar d2t-node-list nil
  "List of nodes.")

(defun d2t-doc-to-texi ()
  "This is the doc to texi converter function.
It calls a bunch of other functions, each of which handles one
particular conversion chore."
  (interactive)
  (setq d2t-level-1-alist   nil ;; initialize variables
	d2t-commands-alist  nil
	d2t-set-show-alist  nil
	d2t-functions-alist nil
	d2t-node-list       nil)
  ;; open the doc file and get some data about its contents
  (d2t-prepare-workspace)
  (d2t-get-level-1)
  (d2t-get-commands)
  (d2t-get-set-show)
  (d2t-get-functions)
  ;; convert the buffer from doc to texi, one element at a time
  (d2t-braces-atsigns)   ;; this must be the first conversion function
  (d2t-sectioning)       ;; chapters, sections, etc
  (d2t-indexing)         ;; index markup
  (d2t-tables)           ;; fix up tables
  (d2t-handle-html)      ;; fix up html markup
  (d2t-first-column)     ;; left justify normal text
  (d2t-comments)         ;; delete comments
  (d2t-enclose-examples) ;; turn indented text into @examples
  (save-excursion        ;; fix a few more things explicitly
    (goto-char (point-min))
    (insert d2t-texi-header)
    (search-forward "@node")
    (beginning-of-line)
    (insert "\n\n" d2t-main-menu "\n\n")
    (search-forward "@node Old_bugs")	; `texinfo-all-menus-update' seems
    (beginning-of-line)			; to miss this one.  how odd.
    (insert "@menu\n* Old_bugs::\t\t\t\n@end menu\n\n")
    (goto-char (point-max))
    (insert d2t-texi-footer))
  (load-library "texinfo") ;; now do the hard stuff with texinfo-mode
  (texinfo-mode)
  (let ((message-log-max 0))
    (texinfo-every-node-update)
    (texinfo-all-menus-update))
  (write-file "gnuplot.texi")  ; save it and done!
  (message "Texinfo output written to gnuplot.texi.  Bye."))

(defun d2t-prepare-workspace ()
  "Create a scratch buffer and populate it with gnuplot.doc."
  (if (get-buffer d2t-work-buffer-name)
      (kill-buffer d2t-work-buffer-name))
  (set-buffer (get-buffer-create d2t-work-buffer-name))
  (insert-file d2t-doc-file-name)
  (goto-char (point-min)))

;;; functions for obtaining lists of nodes in the document

(defun d2t-get-level-1 ()
  "Find all level 1 entries in the doc."
  (save-excursion
    (while (not (eobp))
      (when (re-search-forward "^1 \\(.+\\)$" (point-max) "to_end")
	(beginning-of-line)
	(setq d2t-level-1-alist
	      (append d2t-level-1-alist
		      (list (cons (match-string 1) (point-marker)))))
	(forward-line 1)))))

(defun d2t-get-commands ()
  "Find all commands in the doc."
  (save-excursion
    (let ((alist d2t-level-1-alist) start end)
      (while alist
	(if (string= (caar alist) "Commands")
	    (setq start (cdar  alist) ;; location of "1 Commands"
		  end   (cdadr alist) ;; location of next level 1 heading
		  alist nil)
	  (setq alist (cdr alist))))
      ;;(message "%S %S" start end)
      (goto-char start)
      (while (< (point) end)
	(when (re-search-forward "^2 \\(.+\\)$" (point-max) "to_end")
	  (beginning-of-line)
	  (unless (> (point) end)
	    (setq d2t-commands-alist
		  (append d2t-commands-alist
			  (list (cons (match-string 1) (point-marker))))))
	  (forward-line 1))) )))

(defun d2t-get-set-show ()
  "Find all set-show options in the doc."
  (save-excursion
    (let ((alist d2t-commands-alist) start end)
      (while alist
	(if (string= (caar alist) "set-show")
	    (setq start (cdar  alist) ;; location of "1 set-show"
		  end   (cdadr alist) ;; location of next level 2 heading
		  alist nil)
	  (setq alist (cdr alist))))
      ;;(message "%S %S" start end)
      (goto-char start)
      (while (< (point) end)
	(when (re-search-forward "^3 \\(.+\\)$" (point-max) "to_end")
	  (beginning-of-line)
	  (unless (> (point) end)
	    (setq d2t-set-show-alist
		  (append d2t-set-show-alist
			  (list (cons (match-string 1) (point-marker))))))
	  (forward-line 1))) )))


(defun d2t-get-functions ()
  "Find all functions in the doc."
  (let (begin end)
    (save-excursion			; determine bounds of functions
      (when (re-search-forward "^3 Functions" (point-max) "to_end")
	  (beginning-of-line)
	  (setq begin (point-marker))
	  (forward-line 1)
	  (when (re-search-forward "^3 " (point-max) "to_end")
	    (beginning-of-line)
	    (setq end (point-marker))))
      (goto-char begin)
      (while (< (point) end)
	(when (re-search-forward "^4 \\(.+\\)$" (point-max) "to_end")
	  (beginning-of-line)
	  (unless (> (point) end)
	    (setq d2t-functions-alist
		  (append d2t-functions-alist
			  (list (cons (match-string 1) (point-marker))))))
	  (forward-line 1))) )))

;; buffer manipulation functions

(defun d2t-sectioning ()
  "Find all lines starting with a number.
These are chapters, sections, etc.  Delete these lines and insert the
appropriate sectioning and @node commands."
  (save-excursion
    (while (not (eobp))
      (re-search-forward "^\\([1-9]\\) \\(.+\\)$" (point-max) "to_end")
      (unless (eobp)
	(let* ((number (match-string 1))
	       (word (match-string 2))
	       (node (substitute ?_ ?  word :test 'char-equal))
	       (eol  (save-excursion (end-of-line) (point-marker))))
	  ;; some node names appear twice.  make the second one unique.
	  ;; this will fail to work if a third pops up in the future!
	  (if (member* node d2t-node-list :test 'string=)
	      (setq node (concat node "_")))
	  (setq d2t-node-list (append d2t-node-list (list node)))
	  (beginning-of-line)
	  (delete-region (point-marker) eol)
	  (if (string-match "[1-4]" number) (insert "\n@node " node "\n"))
	  (cond ((string= number "1")
		 (insert "@chapter " word "\n"))
		((string= number "2")
		 (insert "@section " word "\n"))
		((string= number "3")
		 (insert "@subsection " word "\n"))
		((string= number "4")
		 (insert "@subsubsection " word "\n"))
		(t
		 (insert "\n\n@noindent --- " word "\n")) ) )))))

(defun d2t-indexing ()
  "Find all lines starting with a question mark.
These are index references.  Delete these lines and insert the
appropriate indexing commands.  Only index one word ? entries,
comment out the multi-word ? entries."
  (save-excursion
    (while (not (eobp))
      (re-search-forward "^\\\?\\([^ \n]+\\) *$" (point-max) "to_end")
      (unless (eobp)
	(let ((word (match-string 1))
	      (eol  (save-excursion (end-of-line) (point-marker))))
	  (beginning-of-line)
	  (delete-region (point-marker) eol)
	  (insert "@cindex " word "\n")
	  (cond ((assoc word d2t-commands-alist)
		 (insert "@cmindex " word "\n\n"))
		((assoc word d2t-set-show-alist)
		 (insert "@opindex " word "\n\n"))
		((assoc word d2t-functions-alist)
		 (insert "@findex " word "\n\n"))) )))
    (goto-char (point-min))
    (while (not (eobp))
      (re-search-forward "^\\\?" (point-max) "to_end")
      (unless (eobp)
	(if (looking-at "functions? \\(tm_\\w+\\)")
	    (progn
	      (beginning-of-line)
	      (insert "@findex " (match-string 1) "\n@c "))
	  (beginning-of-line)
	  (insert "@c ")))) ))

(defun d2t-comments ()
  "Delete comments and line beginning with # or %.
# and % lines are used in converting tables into various formats."
  (save-excursion
    (while (not (eobp))
      (re-search-forward "^[C#%]" (point-max) "to_end")
      (unless (eobp)
	(let ((eol  (save-excursion (end-of-line)
				    (forward-char 1)
				    (point-marker))))
	  (beginning-of-line)
	  (delete-region (point-marker) eol) )))))

(defun d2t-first-column ()
  "Justify normal text to the 0th column."
  (save-excursion
    (while (not (eobp))
      (if (looking-at "^ [^ \n]")
	  (delete-char 1))
      (forward-line))))

(defun d2t-braces-atsigns ()
  "Prepend @ to @, {, or } everywhere in the doc.
This MUST be the first conversion function called in
`d2t-doc-to-texi'."
  (save-excursion
    (while (not (eobp))
      (re-search-forward "[@{}]" (point-max) "to_end")
      (unless (eobp)
	(backward-char 1)
	(insert "@")
	(forward-char 1)))))

(defun d2t-tables ()
  "Convert doc tables to texi example environments.
That is, use the plain text formatting already in the doc."
  (save-excursion
    (while (not (eobp))
      (re-search-forward "^ *@start table" (point-max) "to_end")
      (unless (eobp)
	(let ((eol  (save-excursion (end-of-line) (point-marker))))
	  (beginning-of-line)
	  (delete-region (point-marker) eol)
	  (insert "@example"))))
    (goto-char (point-min))
    (while (not (eobp))
      (re-search-forward "^ *@end table" (point-max) "to_end")
      (unless (eobp)
	(let ((eol  (save-excursion (end-of-line) (point-marker))))
	  (beginning-of-line)
	  (delete-region (point-marker) eol)
	  (insert "@end example"))))))


(defun d2t-enclose-examples ()
  "Turn indented text in the doc into @examples."
  (save-excursion
    (while (not (eobp))
      (re-search-forward "^ +[^ \n]" (point-max) "to_end")
      (unless (eobp)
	(beginning-of-line)
	(insert "@example\n")
	(forward-line 1)
	(re-search-forward "^[^ ]" (point-max) "to_end")
	(beginning-of-line)
	(insert "@end example\n\n") ))))

(defun d2t-handle-html ()
  "Deal with all of the html markup in the doc."
  (save-excursion
    (while (not (eobp))
      (let ((rx (concat "^" (regexp-quote "^")
			" *\\(<?\\)\\([^ \n]+\\)" )))
	(re-search-forward rx (point-max) "to_end")
	(unless (eobp)
	  (let ((bracket (match-string 1))
		(tag (match-string 2))
		(eol  (save-excursion (end-of-line) (point-marker))))
	    (beginning-of-line)
	    (cond
	     ;; comment out images
	     ((and (string= bracket "<") (string= tag "img"))
	      (insert "@c "))
	     ;; tyepset anchors
	     ((and (string= bracket "<") (string-match "^/?a" tag))
	      (insert "@c fix me!!  "))
	     ;; translate <ul> </ul> to @itemize environment
	     ((and (string= bracket "<") (string-match "^ul" tag))
	      (delete-region (point) eol)
	      (insert "\n@itemize @bullet"))
	     ((and (string= bracket "<") (string-match "/ul" tag))
	      (delete-region (point) eol)
	      (insert "@end itemize\n"))
	     ;; list items
	     ((and (string= bracket "<") (string-match "^li" tag))
	      (delete-char 5)
	      (delete-horizontal-space)
	      (insert "@item\n"))
	     ;; fix up a few miscellaneous things
	     (t ;;(looking-at ".*Terminal Types")
	      (insert "@c ")) )
	    (forward-line))) ))))

;;; doc2texi.el ends here
