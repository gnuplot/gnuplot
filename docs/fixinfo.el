;;; fixinfo.el --- fix gnuplot.info file.

;; Author: Denis Kosygin <kosygin@cims.nyu.edu>
;; Copyright: public domain.

;;; Code:
;(setq backup-inhibited)
(find-file "gnuplot.info")
(goto-char (point-min))
(while (search-forward "" nil t)
  (delete-backward-char 1))
(Info-tagify)
(save-buffer)
;;; fixinfo.el ends here

