;;
;; Simple Emacs mode for the input files.
;;
;; I load this via:
;;
;;     (require 'svm)
;;     (add-to-list 'auto-mode-alist '("\\.in\\'" . svm-mode))
;;

(setq svm-keywords
 '(("^\s*add\\|^\s*sub\\|^\s*mul\\|^\s*div\\|^\s*inc\\|^\s*dec\\|^\s*system\\|^\s*concat\\|^\s*string2int\\|^\s*int2string\\|^\s*cmp\\|^\s*load\\|^\s*print_int\\|^\s*print_str\\|^\s*store\\|^\s*nop\\|^\s*exit" . font-lock-function-name-face)
   ("^\s*goto\\|^\s*jmpnz\\|^\s*jmpz\\|^:[-_A-Za-z0-9]+" . font-lock-warning-face)
  )
)

(define-derived-mode svm-mode fundamental-mode
  (setq font-lock-defaults '(svm-keywords))
  (setq mode-name "svm"))

(provide 'svm)

