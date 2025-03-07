;;; texinfo.el --- major mode for editing Texinfo files  -*- lexical-binding: t; -*-

;; Copyright (C) 1985, 1988-1993, 1996-1997, 2000-2021 Free Software
;; Foundation, Inc.

;; Author: Robert J. Chassell
;; Date:   [See date below for texinfo-version]
;; Maintainer: emacs-devel@gnu.org
;; Keywords: maint, tex, docs

;; This file is part of GNU Emacs.

;; GNU Emacs is free software: you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.

;; GNU Emacs is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with GNU Emacs.  If not, see <https://www.gnu.org/licenses/>.

;;; Todo:

;; - facemenu support.
;; - command completion.

;;; Commentary:

;;; Code:

(eval-when-compile (require 'tex-mode))
(declare-function tex-buffer "tex-mode" ())
(declare-function tex-region "tex-mode" (beg end))
(declare-function tex-send-command "tex-mode")
(declare-function tex-recenter-output-buffer "tex-mode" (linenum))
(declare-function tex-print "tex-mode" (&optional alt))
(declare-function tex-view "tex-mode" ())
(declare-function tex-shell-running "tex-mode" ())
(declare-function tex-kill-job "tex-mode" ())

(defvar outline-heading-alist)

(defvar skeleton-end-newline)

(defgroup texinfo nil
  "Texinfo Mode."
  :link '(custom-group-link :tag "Font Lock Faces group" font-lock-faces)
  :group 'docs)

;;;###autoload
(defcustom texinfo-open-quote (purecopy "``")
  "String inserted by typing \\[texinfo-insert-quote] to open a quotation."
  :type 'string)

;;;###autoload
(defcustom texinfo-close-quote (purecopy "''")
  "String inserted by typing \\[texinfo-insert-quote] to close a quotation."
  :type 'string)

(defcustom texinfo-mode-hook nil
  "Normal hook run when entering Texinfo mode."
  :type 'hook
  :options '(turn-on-auto-fill flyspell-mode))


;;; Autoloads:

(autoload 'kill-compilation
  "compile"
  "Kill the process made by the \\[compile] command."
  t nil)

(require 'texinfo-loaddefs)


;;; Code:

;;; Don't you dare insert any `require' calls at top level in this file--rms.

(defvar texinfo-section-list
  '(("top" 1)
    ("chapter" 2)
    ("section" 3)
    ("subsection" 4)
    ("subsubsection" 5)
    ("unnumbered" 2)
    ("unnumberedsec" 3)
    ("unnumberedsubsec" 4)
    ("unnumberedsubsubsec" 5)
    ("appendix" 2)
    ("appendixsec" 3)
    ("appendixsection" 3)
    ("appendixsubsec" 4)
    ("appendixsubsubsec" 5)
    ("majorheading" 2)
    ("chapheading" 2)
    ("heading" 3)
    ("subheading" 4)
    ("subsubheading" 5))
  "Alist of sectioning commands and their relative level.")

;;; Syntax table

(defvar texinfo-mode-syntax-table
  (let ((st (make-syntax-table)))
    (modify-syntax-entry ?\" "." st)
    (modify-syntax-entry ?\\ "." st)
    (modify-syntax-entry ?@ "\\" st)
    (modify-syntax-entry ?\^q "\\" st)
    (modify-syntax-entry ?\[ "(]" st)
    (modify-syntax-entry ?\] ")[" st)
    (modify-syntax-entry ?{ "(}" st)
    (modify-syntax-entry ?} "){" st)
    (modify-syntax-entry ?\n ">" st)
    (modify-syntax-entry ?\' "w" st)
    st))

;; Written by Wolfgang Bangerth <zcg51122@rpool1.rus.uni-stuttgart.de>
;; To override this example, set either `imenu-generic-expression'
;; or `imenu-create-index-function'.
(defvar texinfo-imenu-generic-expression
  '((nil "^@\\(node\\|anchor\\)[ \t]+\\([^,\n]*\\)" 2)
    ("Chapters" "^@chapter[ \t]+\\(.*\\)$" 1))
  "Imenu generic expression for Texinfo mode.  See `imenu-generic-expression'.")

(defconst texinfo-syntax-propertize-function
  (syntax-propertize-rules
   ("\\(@\\)c\\(omment\\)?\\>" (1 "<"))
   ("^\\(@\\)ignore\\>" (1 "< b"))
   ("^@end ignore\\(\n\\)" (1 "> b")))
  "Syntactic keywords to catch comment delimiters in `texinfo-mode'.")

(defconst texinfo-environments
  '("cartouche" "copying" "defcv" "deffn" "defivar" "defmac"
    "defmethod" "defop" "defopt" "defspec" "deftp" "deftypecv"
    "deftypefn" "deftypefun" "deftypeivar" "deftypemethod"
    "deftypeop" "deftypevar" "deftypevr" "defun" "defvar"
    "defvr" "description" "detailmenu" "direntry" "display"
    "documentdescription" "enumerate" "example" "flushleft"
    "flushright" "format" "ftable" "group" "html" "ifclear" "ifset"
    "ifhtml" "ifinfo" "ifnothtml" "ifnotinfo" "ifnotplaintext"
    "ifnottex" "ifplaintext" "iftex" "ignore" "itemize" "lisp"
    "macro" "menu" "multitable" "quotation" "smalldisplay"
    "smallexample" "smallformat" "smalllisp" "table" "tex"
    "titlepage" "verbatim" "vtable")
  "List of Texinfo environments.")

(defconst texinfo-environment-regexp
  (concat "^@" (regexp-opt (cons "end" texinfo-environments) t) "\\>")
  "Regexp for environment-like Texinfo list commands.
Subexpression 1 is what goes into the corresponding `@end' statement.")

(defface texinfo-heading
  '((t (:inherit font-lock-function-name-face)))
  "Face used for section headings in `texinfo-mode'.")

(defvar texinfo-font-lock-keywords
  `(;; All but the first had an OVERRIDE of t.
    ;; It didn't seem to be any better, and it's slower--simon.
    ;; Robert J. Chassell <bob@gnu.org> says remove this line.
    ;;("\\$\\([^$]*\\)\\$" 1 font-lock-string-face t)
    ("@\\([a-zA-Z]+\\|[^ \t\n]\\)" 1 font-lock-keyword-face) ;commands
    ("^\\*\\([^\n:]*\\)" 1 font-lock-function-name-face t) ;menu items
    ("@\\(emph\\|i\\|sc\\){\\([^}]+\\)" 2 'italic)
    ("@\\(strong\\|b\\){\\([^}]+\\)" 2 'bold)
    ("@\\(kbd\\|key\\|url\\|uref\\){\\([^}]+\\)" 2 font-lock-string-face)
    ;; The following two groups have an OVERRIDE of `keep' because
    ;; their arguments frequently include a @@, and we don't want that
    ;; to overwrite the normal fontification of the argument.
    ("@\\(file\\|email\\){\\([^}]+\\)" 2 font-lock-string-face keep)
    ("@\\(samp\\|code\\|var\\|env\\|command\\|option\\){\\([^}]+\\)"
     2 font-lock-variable-name-face keep)
    ;; @math allows nested braces like @math{2^{12}}
    ("@math{\\([^{}]*{?[^{}]*}?[^{}]*\\)}" 1 font-lock-variable-name-face)
    ("@\\(cite\\|x?ref\\|pxref\\|dfn\\|inforef\\){\\([^}]+\\)"
     2 font-lock-constant-face)
    ("@\\(anchor\\){\\([^}]+\\)" 2 font-lock-type-face)
    ("@\\(dmn\\|acronym\\|value\\){\\([^}]+\\)" 2 font-lock-builtin-face)
    ("@\\(end\\|itemx?\\) +\\(.+\\)" 2 font-lock-keyword-face keep)
    ;; (,texinfo-environment-regexp
    ;;  1 (texinfo-clone-environment (match-beginning 1) (match-end 1)) keep)
    (,(concat "^@" (regexp-opt (mapcar #'car texinfo-section-list) t)
	       ".*\n")
     0 'texinfo-heading t))
  "Additional expressions to highlight in Texinfo mode.")

(defun texinfo-clone-environment (start end)
  (let ((endp nil))
    (save-excursion
      (ignore-errors
	(goto-char start)
	(when (looking-at "end\\Sw+\\(\\sw+\\)")
	  (setq endp t start (match-beginning 1) end (match-end 1)))
	(unless (get-char-property start 'text-clones)
	  (if endp
	      (texinfo-last-unended-begin)
	    (forward-word-strictly 1)
	    (texinfo-next-unmatched-end))
	  (skip-syntax-forward "^w")
	  (when (looking-at
		 (concat (regexp-quote (buffer-substring start end)) "\\>"))
	    (text-clone-create start end 'spread "\\w*")))))))


;;; Keybindings

;;; Keys common both to Texinfo mode and to TeX shell.

(declare-function tex-show-print-queue "tex-mode" ())

(defun texinfo-define-common-keys (keymap)
  "Define the keys both in Texinfo mode and in the texinfo-tex-shell."
  (define-key keymap "\C-c\C-t\C-k"    #'tex-kill-job)
  (define-key keymap "\C-c\C-t\C-x"    #'texinfo-quit-job)
  (define-key keymap "\C-c\C-t\C-l"    #'tex-recenter-output-buffer)
  (define-key keymap "\C-c\C-t\C-d"    #'texinfo-delete-from-print-queue)
  (define-key keymap "\C-c\C-t\C-q"    #'tex-show-print-queue)
  (define-key keymap "\C-c\C-t\C-p"    #'texinfo-tex-print)
  (define-key keymap "\C-c\C-t\C-v"    #'texinfo-tex-view)
  (define-key keymap "\C-c\C-t\C-i"    #'texinfo-texindex)

  (define-key keymap "\C-c\C-t\C-r"    #'texinfo-tex-region)
  (define-key keymap "\C-c\C-t\C-b"    #'texinfo-tex-buffer))

;; Mode documentation displays commands in reverse order
;; from how they are listed in the texinfo-mode-map.

(defvar texinfo-mode-map
  (let ((map (make-sparse-keymap)))

    ;; bindings for `texnfo-tex.el'
    (texinfo-define-common-keys map)

    (define-key map "\"" #'texinfo-insert-quote)

    ;; bindings for `makeinfo.el'
    (define-key map "\C-c\C-m\C-k" #'kill-compilation)
    (define-key map "\C-c\C-m\C-l"
      #'makeinfo-recenter-compilation-buffer)
    (define-key map "\C-c\C-m\C-r" #'makeinfo-region)
    (define-key map "\C-c\C-m\C-b" #'makeinfo-buffer)

    ;; bindings for `texinfmt.el'
    (define-key map "\C-c\C-e\C-r"    #'texinfo-format-region)
    (define-key map "\C-c\C-e\C-b"    #'texinfo-format-buffer)

    ;; AUCTeX-like bindings
    (define-key map "\e\r"		#'texinfo-insert-@item)

    ;; bindings for updating nodes and menus

    (define-key map "\C-c\C-um"   #'texinfo-master-menu)

    (define-key map "\C-c\C-u\C-m"   #'texinfo-make-menu)
    (define-key map "\C-c\C-u\C-n"   #'texinfo-update-node)
    (define-key map "\C-c\C-u\C-e"   #'texinfo-every-node-update)
    (define-key map "\C-c\C-u\C-a"   #'texinfo-all-menus-update)

    (define-key map "\C-c\C-s"     #'texinfo-show-structure)

    (define-key map "\C-c}"          #'up-list)
    ;; FIXME: This is often used for "close block" aka texinfo-insert-@end.
    (define-key map "\C-c]"          #'up-list)
    (define-key map "\C-c/"	     #'texinfo-insert-@end)
    (define-key map "\C-c{"		#'texinfo-insert-braces)

    ;; bindings for inserting strings
    (define-key map "\C-c\C-o"     #'texinfo-insert-block)
    (define-key map "\C-c\C-c\C-d" #'texinfo-start-menu-description)
    (define-key map "\C-c\C-c\C-s" #'texinfo-insert-@strong)
    (define-key map "\C-c\C-c\C-e" #'texinfo-insert-@emph)

    (define-key map "\C-c\C-cv"    #'texinfo-insert-@var)
    (define-key map "\C-c\C-cu"    #'texinfo-insert-@uref)
    (define-key map "\C-c\C-ct"    #'texinfo-insert-@table)
    (define-key map "\C-c\C-cs"    #'texinfo-insert-@samp)
    (define-key map "\C-c\C-cr"    #'texinfo-insert-dwim-@ref)
    (define-key map "\C-c\C-cq"    #'texinfo-insert-@quotation)
    (define-key map "\C-c\C-co"    #'texinfo-insert-@noindent)
    (define-key map "\C-c\C-cn"    #'texinfo-insert-@node)
    (define-key map "\C-c\C-cm"    #'texinfo-insert-@email)
    (define-key map "\C-c\C-ck"    #'texinfo-insert-@kbd)
    (define-key map "\C-c\C-ci"    #'texinfo-insert-@item)
    (define-key map "\C-c\C-cf"    #'texinfo-insert-@file)
    (define-key map "\C-c\C-cx"    #'texinfo-insert-@example)
    (define-key map "\C-c\C-ce"    #'texinfo-insert-@end)
    (define-key map "\C-c\C-cd"    #'texinfo-insert-@dfn)
    (define-key map "\C-c\C-cc"    #'texinfo-insert-@code)

    ;; bindings for environment movement
    (define-key map "\C-c."        #'texinfo-to-environment-bounds)
    (define-key map "\C-c\C-c\C-f" #'texinfo-next-environment-end)
    (define-key map "\C-c\C-c\C-b" #'texinfo-previous-environment-end)
    (define-key map "\C-c\C-c\C-n" #'texinfo-next-environment-start)
    (define-key map "\C-c\C-c\C-p" #'texinfo-previous-environment-start)
    map))

(easy-menu-define texinfo-mode-menu
  texinfo-mode-map
  "Menu used for `texinfo-mode'."
  '("Texinfo"
    ["Insert block"	texinfo-insert-block	t]
    ;; ["Insert node"	texinfo-insert-@node	t]
    "----"
    ["Update All"	(lambda () (interactive) (texinfo-master-menu t))
     :keys "\\[universal-argument] \\[texinfo-master-menu]"]
    ["Update every node" texinfo-every-node-update t]
    ["Update node"	texinfo-update-node	t]
    ["Make Master menu"	texinfo-master-menu	t]
    ["Make menu"	texinfo-make-menu	t]
    ["Update all menus"	texinfo-all-menus-update t]
    "----"
    ["Show structure"	texinfo-show-structure	t]
    ["Format region"	texinfo-format-region	t]
    ["Format buffer"	texinfo-format-buffer	t]
    ["Makeinfo region"	makeinfo-region		t]
    ["Makeinfo buffer"	makeinfo-buffer		t]))


(defun texinfo-filter (section list)
  (let (res)
    (dolist (x list) (if (eq section (cadr x)) (push (car x) res)))
    res))

(defvar texinfo-chapter-level-regexp
  (regexp-opt (texinfo-filter 2 texinfo-section-list))
  "Regular expression matching just the Texinfo chapter level headings.")

(defun texinfo-current-defun-name ()
  "Return the name of the Texinfo node at point, or nil."
  (save-excursion
    (if (re-search-backward "^@node[ \t]+\\([^,\n]+\\)" nil t)
	(match-string-no-properties 1))))

;;; Texinfo mode

;;;###autoload
(define-derived-mode texinfo-mode text-mode "Texinfo"
  "Major mode for editing Texinfo files.

  It has these extra commands:
\\{texinfo-mode-map}

  These are files that are used as input for TeX to make printed manuals
and also to be turned into Info files with \\[makeinfo-buffer] or
the `makeinfo' program.  These files must be written in a very restricted and
modified version of TeX input format.

  Editing commands are like `text-mode' except that the syntax table is
set up so expression commands skip Texinfo bracket groups.  To see
what the Info version of a region of the Texinfo file will look like,
use \\[makeinfo-region], which runs `makeinfo' on the current region.

  You can show the structure of a Texinfo file with \\[texinfo-show-structure].
This command shows the structure of a Texinfo file by listing the
lines with the @-sign commands for @chapter, @section, and the like.
These lines are displayed in another window called the *Occur* window.
In that window, you can position the cursor over one of the lines and
use \\[occur-mode-goto-occurrence], to jump to the corresponding spot
in the Texinfo file.

  In addition, Texinfo mode provides commands that insert various
frequently used @-sign commands into the buffer.  You can use these
commands to save keystrokes.  And you can insert balanced braces with
\\[texinfo-insert-braces] and later use the command \\[up-list] to
move forward past the closing brace.

Also, Texinfo mode provides functions for automatically creating or
updating menus and node pointers.  These functions

  * insert the `Next', `Previous' and `Up' pointers of a node,
  * insert or update the menu for a section, and
  * create a master menu for a Texinfo source file.

Here are the functions:

    `texinfo-update-node'                \\[texinfo-update-node]
    `texinfo-every-node-update'          \\[texinfo-every-node-update]
    `texinfo-sequential-node-update'

    `texinfo-make-menu'                  \\[texinfo-make-menu]
    `texinfo-all-menus-update'           \\[texinfo-all-menus-update]
    `texinfo-master-menu'

    `texinfo-indent-menu-description' (column &optional region-p)

The `texinfo-column-for-description' variable specifies the column to
which menu descriptions are indented.

Passed an argument (a prefix argument, if interactive), the
`texinfo-update-node' and `texinfo-make-menu' functions do their jobs
in the region.

To use the updating commands, you must structure your Texinfo file
hierarchically, such that each `@node' line, with the exception of the
Top node, is accompanied by some kind of section line, such as an
`@chapter' or `@section' line.

If the file has a `top' node, it must be called `top' or `Top' and
be the first node in the file.

Entering Texinfo mode calls the value of `text-mode-hook', and then the
value of `texinfo-mode-hook'."
  (setq-local page-delimiter
	      (concat "^@node [ \t]*[Tt]op\\|^@\\("
		      texinfo-chapter-level-regexp
		      "\\)\\>"))
  (setq-local require-final-newline mode-require-final-newline)
  (setq-local indent-tabs-mode nil)
  (setq-local paragraph-start (concat "@[a-zA-Z]*[ \n]\\|"
				      paragraph-start))
  (setq-local fill-paragraph-function 'texinfo--fill-paragraph)
  (setq-local sentence-end-base "\\(@\\(end\\)?dots{}\\|[.?!]\\)[]\"'”)}]*")
  (setq-local fill-column 70)
  (setq-local beginning-of-defun-function #'texinfo--beginning-of-defun)
  (setq-local end-of-defun-function #'texinfo--end-of-defun)
  (setq-local comment-start "@c ")
  (setq-local comment-start-skip "@c +\\|@comment +")
  (setq-local words-include-escapes t)
  (setq-local imenu-generic-expression texinfo-imenu-generic-expression)
  (setq imenu-case-fold-search nil)
  (setq font-lock-defaults
	'(texinfo-font-lock-keywords nil nil nil backward-paragraph))
  (setq-local syntax-propertize-function texinfo-syntax-propertize-function)
  (setq-local add-log-current-defun-function #'texinfo-current-defun-name)

  ;; Outline settings.
  (setq-local outline-heading-alist
	      ;; We should merge `outline-heading-alist' and
	      ;; `texinfo-section-list'.  But in the mean time, let's
	      ;; just generate one from the other.
	      (mapcar (lambda (x) (cons (concat "@" (car x)) (cadr x)))
		      texinfo-section-list))
  (setq-local outline-regexp
	      (concat (regexp-opt (mapcar #'car outline-heading-alist) t)
		      "\\>"))

  (setq-local tex-start-of-header "%\\*\\*start")
  (setq-local tex-end-of-header "%\\*\\*end")
  (setq-local tex-first-line-header-regexp "^\\\\input")
  (setq-local tex-trailer "@bye\n")

  ;; Prevent skeleton.el from adding a newline to each inserted
  ;; skeleton.  Those which do want a newline do that explicitly in
  ;; their define-skeleton form.
  (setq-local skeleton-end-newline nil)

  ;; Prevent filling certain lines, in addition to ones specified by
  ;; the user.
  (setq-local auto-fill-inhibit-regexp
	      (let ((prevent-filling "^@\\(def\\|multitable\\)"))
		(if (null auto-fill-inhibit-regexp)
		    prevent-filling
		  (concat auto-fill-inhibit-regexp "\\|" prevent-filling)))))

(defvar texinfo-fillable-commands '("@noindent")
  "A list of commands that can be filled.")

(defun texinfo--fill-paragraph (justify)
  "Function to fill a paragraph in `texinfo-mode'."
  (let ((command-re "\\(@[a-zA-Z]+\\)[ \t\n]"))
    (catch 'no-fill
      (save-restriction
        ;; First check whether we're on a command line that can be
        ;; filled by itself.
        (or
         (save-excursion
           (beginning-of-line)
           (when (looking-at command-re)
             (let ((command (match-string 1)))
               (if (member command texinfo-fillable-commands)
                   (progn
                     (narrow-to-region (point) (progn (forward-line 1) (point)))
                     t)
                 (throw 'no-fill nil)))))
         ;; We're not on such a line, so fill the region.
         (save-excursion
           (let ((regexp (concat command-re "\\|^[ \t]*$\\|\f")))
             (narrow-to-region
              (if (re-search-backward regexp nil t)
                  (progn
                    (forward-line 1)
                    (point))
                (point-min))
              (if (re-search-forward regexp nil t)
                  (match-beginning 0)
                (point-max)))
             (goto-char (point-min)))))
        ;; We've now narrowed to the region we want to fill.
        (let ((fill-paragraph-function nil)
              (adaptive-fill-mode nil))
          (fill-paragraph justify))))
    t))

(defun texinfo--beginning-of-defun (&optional arg)
  "Go to the previous @node line."
  (while (and (> arg 0)
              (re-search-backward "^@node " nil t))
    (setq arg (1- arg))))

(defun texinfo--end-of-defun ()
  "Go to the start of the next @node line."
  (when (looking-at-p "@node")
    (forward-line))
  (if (re-search-forward "^@node " nil t)
      (goto-char (match-beginning 0))
    (goto-char (point-max))))


;;; Insert string commands

(defvar texinfo-block-default "example")

(define-skeleton texinfo-insert-block
  "Create a matching pair @<cmd> .. @end <cmd> at point.
Puts point on a blank line between them."
  (setq texinfo-block-default
	(completing-read (format "Block name [%s]: " texinfo-block-default)
			 texinfo-environments
			 nil nil nil nil texinfo-block-default))
  \n "@" str
  ;; Blocks that take parameters: all the def* blocks take parameters,
  ;;  plus a few others.
  (if (or (string-match "\\`def" str)
          (member str '("table" "ftable" "vtable")))
      '(nil " " -))
  \n _ \n "@end " str \n \n)

(defun texinfo-inside-macro-p (macro &optional bound)
  "Non-nil if inside a macro matching the regexp MACRO."
  (condition-case nil
      (save-excursion
	(save-restriction
	  (narrow-to-region bound (point))
	  (while (progn
		   (up-list -1)
		   (not (condition-case nil
			    (save-excursion
			      (backward-sexp 1)
			      (looking-at macro))
			  (scan-error nil)))))
	  t))
    (scan-error nil)))

(defun texinfo-inside-env-p (env &optional bound)
  "Non-nil if inside an environment matching the regexp @ENV."
  (save-excursion
    (and (re-search-backward (concat "@\\(end\\s +\\)?" env) bound t)
	 (not (match-end 1)))))

(defvar texinfo-enable-quote-macros "@\\(code\\|samp\\|kbd\\)\\>")
(defvar texinfo-enable-quote-envs
  '("example\\>" "smallexample\\>" "lisp\\>"))
(defun texinfo-insert-quote (&optional arg)
  "Insert the appropriate quote mark for Texinfo.
Usually inserts the value of `texinfo-open-quote' (normally \\=`\\=`) or
`texinfo-close-quote' (normally \\='\\='), depending on the context.
With prefix argument or inside @code or @example, inserts a plain \"."
  (interactive "*P")
  (let ((top (or (save-excursion (re-search-backward "@node\\>" nil t))
		 (point-min))))
    (if (or arg
	    (= (preceding-char) ?\\)
	    (save-excursion
              ;; Might be near the start of a (narrowed) buffer.
              (ignore-errors (backward-char (length texinfo-open-quote)))
	      (when (or (looking-at texinfo-open-quote)
			(looking-at texinfo-close-quote))
		(delete-char (length texinfo-open-quote))
		t))
	    (texinfo-inside-macro-p texinfo-enable-quote-macros top)
	    (let ((in-env nil))
	      (dolist (env texinfo-enable-quote-envs in-env)
		(if (texinfo-inside-env-p env top)
		    (setq in-env t)))))
	(self-insert-command (prefix-numeric-value arg))
      (insert
       (if (or (bobp)
               (memq (char-syntax (preceding-char)) '(?\( ?> ?\s)))
	   texinfo-open-quote
	 texinfo-close-quote)))))

;; The following texinfo-insert-@end command not only inserts a SPC
;; after the @end, but tries to find out what belongs there.  It is
;; not very smart: it does not understand nested lists.

(defun texinfo-last-unended-begin ()
  (while (and (re-search-backward texinfo-environment-regexp)
	      (looking-at "@end"))
    (texinfo-last-unended-begin)))

(defun texinfo-next-unmatched-end ()
  (while (and (re-search-forward texinfo-environment-regexp)
	      (save-excursion
		(goto-char (match-beginning 0))
		(not (looking-at "@end"))))
    (texinfo-next-unmatched-end)))

(define-skeleton texinfo-insert-@end
  "Insert the matching `@end' for the last Texinfo command that needs one."
	 (ignore-errors
	   (save-excursion
             (backward-word-strictly 1)
	     (texinfo-last-unended-begin)
      (or (match-string 1) '-)))
  \n "@end " str \n \n)

(define-skeleton texinfo-insert-braces
  "Make a pair of braces and be poised to type inside of them.
Use \\[up-list] to move forward out of the braces."
  nil
  "{" _ "}")

(define-skeleton texinfo-insert-@code
  "Insert a `@code{...}' command in a Texinfo buffer.
A numeric argument says how many words the braces should surround.
The default is not to surround any existing words with the braces."
  nil
  "@code{" _ "}")

(define-skeleton texinfo-insert-@dfn
  "Insert a `@dfn{...}' command in a Texinfo buffer.
A numeric argument says how many words the braces should surround.
The default is not to surround any existing words with the braces."
  nil
  "@dfn{" _ "}")

(define-skeleton texinfo-insert-@email
  "Insert a `@email{...}' command in a Texinfo buffer.
A numeric argument says how many words the braces should surround.
The default is not to surround any existing words with the braces."
  nil
  "@email{" _ "}")

(define-skeleton texinfo-insert-@emph
  "Insert a `@emph{...}' command in a Texinfo buffer.
A numeric argument says how many words the braces should surround.
The default is not to surround any existing words with the braces."
  nil
  "@emph{" _ "}")

(define-skeleton texinfo-insert-@example
  "Insert the string `@example' in a Texinfo buffer."
  nil
  \n "@example" \n \n)

(define-skeleton texinfo-insert-@file
  "Insert a `@file{...}' command in a Texinfo buffer.
A numeric argument says how many words the braces should surround.
The default is not to surround any existing words with the braces."
  nil
  "@file{" _ "}")

(define-skeleton texinfo-insert-@item
  "Insert the string `@item' in a Texinfo buffer.
If in a table defined by @table, follow said string with a space.
Otherwise, follow with a newline."
  nil
  \n "@item"
	  (if (equal (ignore-errors
		      (save-excursion
			(texinfo-last-unended-begin)
			(match-string 1)))
		     "table")
      " " '\n)
  _ \n)

(define-skeleton texinfo-insert-@kbd
  "Insert a `@kbd{...}' command in a Texinfo buffer.
A numeric argument says how many words the braces should surround.
The default is not to surround any existing words with the braces."
  nil
  "@kbd{" _ "}")

(define-skeleton texinfo-insert-@node
  "Insert the string `@node' in a Texinfo buffer.
Insert a comment on the following line indicating the order of
arguments to @node.  Insert a carriage return after the comment line.
Leave point after `@node'."
  nil
  \n "@node " _ \n)

(define-skeleton texinfo-insert-@noindent
  "Insert the string `@noindent' in a Texinfo buffer."
  nil
  \n "@noindent" \n)

(define-skeleton texinfo-insert-@quotation
  "Insert the string `@quotation' in a Texinfo buffer."
  \n "@quotation" \n _ \n)

(define-skeleton texinfo-insert-dwim-@ref
  "Insert appropriate `@pxref{...}', `@xref{}', or `@ref{}' command.

Looks at text around point to decide what to insert; an unclosed
preceding open parenthesis results in '@pxref{}', point at the
beginning of a sentence or at (point-min) yields '@xref{}', any
other location (including inside a word), will result in '@ref{}'
at the nearest previous whitespace or beginning-of-line.  A
numeric argument says how many words the braces should surround.
The default is not to surround any existing words with the
braces."
  nil
  (cond
   ;; parenthesis
   ((looking-back "([^)]*" (point-at-bol 0))
    "@pxref{")
   ;; beginning of sentence or buffer
   ((or (looking-back (sentence-end) (point-at-bol 0))
        (= (point) (point-min)))
    "@xref{")
   ;; bol or eol
   ((looking-at "^\\|$")
    "@ref{")
   ;; inside word
   ((not (eq (char-syntax (char-after)) ? ))
    (skip-syntax-backward "^ " (point-at-bol))
    "@ref{")
   ;; everything else
   (t
    "@ref{"))
  _ "}")

(define-skeleton texinfo-insert-@samp
  "Insert a `@samp{...}' command in a Texinfo buffer.
A numeric argument says how many words the braces should surround.
The default is not to surround any existing words with the braces."
  nil
  "@samp{" _ "}")

(define-skeleton texinfo-insert-@strong
  "Insert a `@strong{...}' command in a Texinfo buffer.
A numeric argument says how many words the braces should surround.
The default is not to surround any existing words with the braces."
  nil
  "@strong{" _ "}")

(define-skeleton texinfo-insert-@table
  "Insert the string `@table' in a Texinfo buffer."
  nil
  \n "@table " _ \n)

(define-skeleton texinfo-insert-@var
  "Insert a `@var{}' command in a Texinfo buffer.
A numeric argument says how many words the braces should surround.
The default is not to surround any existing words with the braces."
  nil
  "@var{" _ "}")

(define-skeleton texinfo-insert-@uref
  "Insert a `@uref{}' command in a Texinfo buffer.
A numeric argument says how many words the braces should surround.
The default is not to surround any existing words with the braces."
  nil
  "@uref{" _ "}")
(defalias 'texinfo-insert-@url #'texinfo-insert-@uref)

;;; Texinfo file structure

(defun texinfo-show-structure (&optional nodes-too)
  "Show the structure of a Texinfo file.
List the lines in the file that begin with the @-sign commands for
@chapter, @section, and the like.

With optional argument (prefix if interactive), list both the lines
with @-sign commands for @chapter, @section, and the like, and list
@node lines.

Lines with structuring commands beginning in them are displayed in
another buffer named `*Occur*'.  In that buffer, you can move point to
one of those lines and then use
\\<occur-mode-map>\\[occur-mode-goto-occurrence],
to jump to the corresponding spot in the Texinfo source file."

  (interactive "P")
  ;; First, remember current location
  (let (current-location)
    (save-excursion
      (end-of-line)            ; so as to find section on current line
      (if (re-search-backward
           ;; do not require `texinfo-section-types-regexp' in texnfo-upd.el
           "^@\\(chapter \\|sect\\|subs\\|subh\\|unnum\\|major\\|chapheading \\|heading \\|appendix\\)"
           nil t)
          (setq current-location
                (progn
                  (beginning-of-line)
                  (buffer-substring (point) (progn (end-of-line) (point)))))
        ;; else point is located before any section command.
        (setq current-location "tex")))
    ;; Second, create and format an *Occur* buffer
    (save-excursion
      (goto-char (point-min))
      (occur (concat "^\\(?:" (if nodes-too "@node\\>\\|")
		     outline-regexp "\\)")))
    (pop-to-buffer "*Occur*")
    (goto-char (point-min))
    (let ((inhibit-read-only t))
      (flush-lines "-----")
      ;; Now format the "*Occur*" buffer to show the structure.
      ;; Thanks to ceder@signum.se (Per Cederqvist)
      (goto-char (point-max))
      (let (level)
        (while (re-search-backward "^ *[0-9]*:@\\(\\sw+\\)" nil 0)
          (goto-char (1- (match-beginning 1)))
          (setq level
                (or (cadr (assoc (match-string 1) texinfo-section-list)) 2))
          (indent-to-column (+ (current-column) (* 4 (- level 2))))
          (beginning-of-line))))
    ;; Third, go to line corresponding to location in source file
    ;; potential bug: two exactly similar `current-location' lines ...
    (goto-char (point-min))
    (re-search-forward current-location nil t)
    (beginning-of-line)
    ))


;;; The  tex  and  print  function definitions:

(defcustom texinfo-texi2dvi-command "texi2dvi"
  "Command used by `texinfo-tex-buffer' to run TeX and texindex on a buffer."
  :type 'string)

(defcustom texinfo-texi2dvi-options ""
  "Command line options for `texinfo-texi2dvi-command'."
  :type 'string
  :version "28.1")

(defcustom texinfo-tex-command "tex"
  "Command used by `texinfo-tex-region' to run TeX on a region."
  :type 'string)

(defcustom texinfo-texindex-command "texindex"
  "Command used by `texinfo-texindex' to sort unsorted index files."
  :type 'string)

(defcustom texinfo-delete-from-print-queue-command "lprm"
  "Command string used to delete a job from the line printer queue.
Command is used by \\[texinfo-delete-from-print-queue] based on
number provided by a previous \\[tex-show-print-queue]
command."
  :type 'string)

(defvar texinfo-tex-trailer "@bye"
  "String appended after a region sent to TeX by `texinfo-tex-region'.")

(defun texinfo-tex-region (beg end)
  "Run TeX on the current region.
This works by writing a temporary file (`tex-zap-file') in the directory
that is the value of `tex-directory', then running TeX on that file.

The first line of the buffer is copied to the
temporary file; and if the buffer has a header, it is written to the
temporary file before the region itself.  The buffer's header is all lines
between the strings defined by `tex-start-of-header' and `tex-end-of-header'
inclusive.  The header must start in the first 100 lines.

The value of `texinfo-tex-trailer' is appended to the temporary
file after the region."
  (interactive "r")
  (require 'tex-mode)
  (let ((tex-command texinfo-tex-command)
	(tex-trailer texinfo-tex-trailer))
    (tex-region beg end)))

(defun texinfo-tex-buffer ()
  "Run TeX on visited file, once or twice, to make a correct `.dvi' file."
  (interactive)
  (require 'tex-mode)
  (let ((tex-command texinfo-texi2dvi-command)
	(tex-start-options texinfo-texi2dvi-options)
	;; Disable tex-start-commands.  texi2dvi would not understand
	;; anything specified here.
        (tex-start-commands ""))
    (tex-buffer)))

(defun texinfo-texindex ()
  "Run `texindex' on unsorted index files.
The index files are made by \\[texinfo-tex-region] or \\[texinfo-tex-buffer].
This runs the shell command defined by `texinfo-texindex-command'."
  (interactive)
  (require 'tex-mode)
  (tex-send-command texinfo-texindex-command (concat tex-zap-file ".??"))
  ;; alternatively
  ;; (send-string "tex-shell"
  ;;              (concat texinfo-texindex-command
  ;;                      " " tex-zap-file ".??" "\n"))
  (tex-recenter-output-buffer nil))

(defun texinfo-tex-print ()
  "Print `.dvi' file made by \\[texinfo-tex-region] or \\[texinfo-tex-buffer].
This runs the shell command defined by `tex-dvi-print-command'."
  (interactive)
  (require 'tex-mode)
  (tex-print))

(defun texinfo-tex-view ()
  "View `.dvi' file made by \\[texinfo-tex-region] or \\[texinfo-tex-buffer].
This runs the shell command defined by `tex-dvi-view-command'."
  (interactive)
  (require 'tex-mode)
  (tex-view))

(defun texinfo-quit-job ()
  "Quit currently running TeX job, by sending an `x' to it."
  (interactive)
  (if (not (get-process "tex-shell"))
      (error "No TeX shell running"))
  (tex-send-command "x"))
;; alternatively:
;; save-excursion
;;   (set-buffer (get-buffer "*tex-shell*"))
;;   (goto-char (point-max))
;;   (insert "x")
;;   (comint-send-input)

(defun texinfo-delete-from-print-queue (job-number)
  "Delete job from the line printer spooling queue.
You are prompted for the job number (use a number shown by a previous
\\[tex-show-print-queue] command)."
  (interactive "nPrinter job number for deletion: ")
  (require 'tex-mode)
  (if (tex-shell-running)
      (tex-kill-job)
    (tex-start-shell))
  (tex-send-command texinfo-delete-from-print-queue-command job-number)
  ;; alternatively
  ;; (send-string "tex-shell"
  ;;              (concat
  ;;               texinfo-delete-from-print-queue-command
  ;;               " "
  ;;               job-number"\n"))
  (tex-recenter-output-buffer nil))

(defun texinfo-to-environment-bounds ()
  "Move point alternately to the start and end of a Texinfo environment.
Do nothing when outside of an environment.  This command does not
handle nested environments."
  (interactive)
  (cond ((save-excursion
	   (forward-line 0)
	   (looking-at texinfo-environment-regexp))
	 (if (save-excursion
	       (forward-line 0)
	       (looking-at "^@end"))
	     (texinfo-previous-environment-start)
	   (texinfo-next-environment-end)))
	((save-excursion
	   (and (re-search-backward texinfo-environment-regexp nil t)
		(not (looking-at "^@end"))))
	 (texinfo-previous-environment-start))
	;; Otherwise, point is outside of an environment, so do nothing.
	))

(defun texinfo-next-environment-start ()
  "Move forward to the beginning of a Texinfo environment."
  (interactive)
  (if (looking-at texinfo-environment-regexp)
      (forward-line 1))
  (while (and (re-search-forward texinfo-environment-regexp nil t)
	      (save-excursion
		(goto-char (match-beginning 0))
		(looking-at "@end"))))
  (if (save-excursion
	(forward-line 0)
	(looking-at texinfo-environment-regexp))
      (forward-line 0)))

(defun texinfo-previous-environment-start ()
  "Move back to the beginning of the previous Texinfo environment."
  (interactive)
  (while (and (re-search-backward texinfo-environment-regexp nil t)
	      (save-excursion
		(goto-char (match-beginning 0))
		(looking-at "@end")))))

(defun texinfo-next-environment-end ()
  "Move forward to the beginning of the next @end line of an environment."
  (interactive)
  (if (looking-at "^@end")
      (forward-line 1))
  (while (and (re-search-forward texinfo-environment-regexp nil t)
	      (save-excursion
		(goto-char (match-beginning 0))
		(not (looking-at "^@end")))))
  (if (save-excursion
	(forward-line 0)
	(looking-at "^@end"))
      (forward-line 0)))

(defun texinfo-previous-environment-end ()
  "Move backward to the beginning of the next @end line of an environment."
  (interactive)
  (while (and (re-search-backward texinfo-environment-regexp nil t)
	      (save-excursion
		(goto-char (match-beginning 0))
		(not (looking-at "@end"))))))

(provide 'texinfo)

;;; texinfo.el ends here
