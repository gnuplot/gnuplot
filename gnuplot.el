;;
;; $Id: gnuplot.el%v 3.50 1993/07/09 05:35:24 woo Exp $
;;
;;
; 
; gnu-plot.el - Definitions of GNU-PLOT mode for emacs editor.
; 
; Author:	Gershon Elber
; 		Computer Science Dept.
; 		University of Utah
; Date:	Tue May 14 1991
; Copyright (c) 1991, 1992, Gershon Elber
;
; This file defines an environment to run edit and execute GNU-PLOT programs.
; Such a program should have a '.gp' extension in order it to be in
; gnu-plot-mode major mode. Two new functions are provided to communicate
; between the editted file and the plotting program:
; 1. send-line-to-gnu-plot - sends a single line to the plotting program for
;    execution. The line sent is the line the cursor is on,
;    Bounded to Meta-E by default.
; 2. send-region-to-gnu-plot - sends the region from the current mark
;    (mark-marker) to current position (point-marker) to the plotting program.
;    This function is convenient for sending a large block of commands.
;    Bounded to Meta-R by default.
; Both functions checks for existance of a buffer named gnu-plot-program
; and a process named "gnu-plot" hooked to it, and will restart a new process
; or buffer if none exists. The program to execute as process "gnu-plot" is
; defined by the gnu-plot-program constant below.
;

(defvar gnu-plot-program "gnuplot"
  "*The executable to run for gnu-plot-program buffer.")

(defvar gnu-plot-echo-program t
  "*Control echo of executed commands to gnu-plot-program buffer.")

(defvar gnu-plot-mode-map nil "")
(if gnu-plot-mode-map
    ()
  (setq gnu-plot-mode-map (make-sparse-keymap))
  (define-key gnu-plot-mode-map "\M-e" 'send-line-to-gnu-plot)
  (define-key gnu-plot-mode-map "\M-r" 'send-region-to-gnu-plot))

;;;
;;; Define the gnu-plot-mode
;;;
(defun gnu-plot-mode ()
  "Major mode for editing and executing GNU-PLOT files.

see send-line-to-gnu-plot and send-region-to-gnu-plot for more."
  (interactive)
  (use-local-map gnu-plot-mode-map)
  (setq major-mode 'gnu-plot-mode)
  (setq mode-name "Gnu-Plot")
  (run-hooks 'gnu-plot-mode-hook))

;;;
;;; Define send-line-to-gnu-plot - send from current cursor position to next
;;; semicolin detected.
;;;
(defun send-line-to-gnu-plot ()
  "Sends one line of code from current buffer to the GNU-PLOT program.

Use to execute a line in the GNU-PLOT plotting program. The line sent is
the line the cursor (point) is on.

The GNU-PLOT plotting program buffer name is gnu-plot-program and the 
process name is 'gnu-plot'. If none exists, a new one is created.

The name of the gnu-plot program program to execute is stored in
gnu-plot-program variable and may be changed."
  (interactive)
  (if (equal major-mode 'gnu-plot-mode)
    (progn
      (make-gnu-plot-buffer)        ; In case we should start a new one.
      (beginning-of-line)
      (let ((start-mark (point-marker)))
	(next-line 1)
	(let* ((crnt-buffer (buffer-name))
	       (end-mark (point-marker))
	       (string-copy (buffer-substring start-mark end-mark)))
	  (switch-to-buffer-other-window (get-buffer "gnu-plot-program"))
	  (end-of-buffer)
	  (if gnu-plot-echo-program
	    (insert string-copy))
	  (set-marker (process-mark (get-process "gnu-plot")) (point-marker))
	  (if (not (pos-visible-in-window-p))
	    (recenter 3))
	  (switch-to-buffer-other-window (get-buffer crnt-buffer))
	  (process-send-region "gnu-plot" start-mark end-mark)
	  (goto-char end-mark))))
    (message "Should be invoked in gnu-plot-mode only.")))

;;;
;;; Define send-region-to-gnu-plot - send from current cursor position to
;;; current marker.
;;;
(defun send-region-to-gnu-plot ()
  "Sends a region of code from current buffer to the GNU-PLOT program.

When this function is invoked on an GNU-PLOT file it send the region
from current point to current mark to the gnu-plot plotting program.

The GNU-PLOT plotting program buffer name is gnu-plot-program and the
process name is 'gnu-plot'. If none exists, a new one is created.

The name of the gnu-plot program program to execute is stored in
gnu-plot-program variable and may be changed."
  (interactive)
  (if (equal major-mode 'gnu-plot-mode)
    (progn
      (make-gnu-plot-buffer)     ; In case we should start a new one.
      (copy-region-as-kill (mark-marker) (point-marker))
      (let ((crnt-buffer (buffer-name)))
	(switch-to-buffer-other-window (get-buffer "gnu-plot-program"))
	(end-of-buffer)
	(if gnu-plot-echo-program
	  (yank))
	(set-marker (process-mark (get-process "gnu-plot")) (point-marker))
	(if (not (pos-visible-in-window-p))
	  (recenter 3))
	(switch-to-buffer-other-window (get-buffer crnt-buffer))
	(process-send-region "gnu-plot" (mark-marker) (point-marker))))
    (message "Should be invoked in gnu-plot-mode only.")))

;;;
;;; Switch to "gnu-plot-program" buffer if exists. If not, creates one and
;;; execute the program defined by gnu-plot-program.
;;;
(defun make-gnu-plot-buffer ()
  "Switch to gnu-plot-program buffer or create one if none exists"
  (interactive)
  (if (get-buffer "gnu-plot-program")
    (if (not (get-process "gnu-plot"))
      (progn
	(message "Starting GNU-PLOT plotting program...")
	(start-process "gnu-plot" "gnu-plot-program" gnu-plot-program)
	(process-send-string "gnu-plot" "\n")
	(message "Done.")))
    (progn
      (message "Starting GNU-PLOT plotting program...")
      (start-process "gnu-plot" "gnu-plot-program" gnu-plot-program)
      (process-send-string "gnu-plot" "\n")
      (message "Done."))))

;;;
;;; Autoload gnu-plot-mode on any file with gp extension. 
;;;
(setq auto-mode-alist (append '(("\\.gp$" . gnu-plot-mode))
			      auto-mode-alist))
