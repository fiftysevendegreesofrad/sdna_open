;sDNA software for spatial network analysis 
;Copyright (C) 2011-2019 Cardiff University

;This program is free software: you can redistribute it and/or modify
;it under the terms of the GNU General Public License as published by
;the Free Software Foundation, either version 3 of the License, or
;(at your option) any later version.

;This program is distributed in the hope that it will be useful,
;but WITHOUT ANY WARRANTY; without even the implied warranty of
;MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;GNU General Public License for more details.

;You should have received a copy of the GNU General Public License
;along with this program.  If not, see <https://www.gnu.org/licenses/>.
(defun check-sdna-return (x)
  (if (= x nil)
    (progn
    (alert "sDNA Com Wrapper is not installed properly.  To fix this problem, try closing all windows then re-running sDNA License Manager.")
    (quit)
    )
  )
)

(defun check-sdnaprog-return (x)
  (if (= x nil)
    (progn
    (alert "sDNA Progress Bar is not installed properly.  To fix this problem, try closing all windows then re-running sDNA License Manager.")
    (quit)
    )
  )
)

(defun get_entity_with_sdna_data (entity)
  (entget entity '("sdna"))
)

(defun extract_sdna_xdata_from_entity (e / sdna_data_group)
  (setq sdna_data_group (assoc -3 e))
  (if sdna_data_group
  	(xdata->assoclist (cdr(cadr(assoc -3 e))))
        nil
  )  
)

(defun set_entity_sdna_xdata (entity data)
  (entmod (create_or_replace_item_in_assoclist
	    entity
	    -3
	    (list (cons "sdna" (assoclist->xdata data)))
	  )
  )
)


(defun create_or_replace_item_in_assoclist (assoclist key value / old_pair new_pair)
 (progn
  (setq old_pair (assoc key assoclist))
  (setq new_pair (cons key value))
  (if old_pair
    (subst new_pair old_pair assoclist)
    (cons new_pair assoclist)
  )
 )
)

(defun get_item_from_assoclist (lst key) (cdr (assoc key lst)))

(defun read_sdna_param_from_enlist (paramnumber entlist)
  (get_item_from_assoclist (extract_sdna_xdata_from_entity entlist) paramnumber)
)

(defun set_sdna_params (entname param_names param_values / entity sdna_xdata)
  (progn
    (setq entity (get_entity_with_sdna_data entname)
	  sdna_xdata (extract_sdna_xdata_from_entity entity)
    )
    (mapcar (quote (lambda (name value)
		     (setq sdna_xdata
			    (create_or_replace_item_in_assoclist sdna_xdata name value)
		     )
		   )
	    )
	    param_names param_values
    )
    (set_entity_sdna_xdata entity sdna_xdata)
  )
)
    

(defun available_indices (lst)
  (if (null lst)
    nil
    (cons (caar lst) (available_indices (cdr lst)))
  )
)

(defun make_polyline (xs ys colour)
	  (progn
	    	(setq coords
		       (mapcar
				(quote (lambda (x)
				  (cons 10 x)
				))
				(zip xs ys)
			)
		)
	    	(setq nature (if isisland "Traffic Island Link" "Normal"))
		(setq linedef (list '(0 . "LWPOLYLINE")
		               '(100 . "AcDbEntity")
		               '(100 . "AcDbPolyline")
		               '(43 . 0); constant width
			       (cons 62 colour)
			       (cons 90 (length coords))
			      )
		 )
		(setq linedef (append linedef coords))
		(entmake linedef)
	        (entlast)
	  )
)

(defun line-closed (entity) (= :vlax-true (vla-get-closed (vlax-ename->vla-object entity))))

(defun read-line-or-unlink	(net	     id		 entity	     allow_unlinks / line_closed
		 sdnaparams  startelev	 endelev     nature
		 xcoords     ycoords	 enlist	     xcoord_sa
		 ycoord_sa   coordlength sdnadata    isisland coords_needed
		)
  (progn
    (setq enlist (get_entity_with_sdna_data entity))
    (setq sdna_xdata (extract_sdna_xdata_from_entity enlist))
    (setq
      startelev	(get_item_from_assoclist sdna_xdata "startelev")
      endelev	(get_item_from_assoclist sdna_xdata "endelev")
      nature	(get_item_from_assoclist sdna_xdata "nature")
      isisland	(or (= nature "Traffic Island Link")
		    (= nature "Traffic Island Link At Junction")
		)
    )
    (if	(= startelev nil)		; convert to int
      (setq startelev 0)
    )
    (if	(= endelev nil)			; convert to int
      (setq endelev 0)
    )
    (if	(= isisland nil)		; convert to variant_bool
      (setq isisland 0)
    )

    (setq line_closed (line-closed entity))
    (if (and line_closed (not allow_unlinks))
      (progn
	(princ "Error in input data: unlink (closed polyline) found, not expected")
	(princ)
	(exit)
	)
      )
    
    (setq coordlength (cdr (assoc 90 enlist)))
    (setq coords_needed (if line_closed 3 2))
    (if (< coordlength coords_needed)
      (progn
	(princ "Error in input data (polyline with <2 or polygon with <3 points)")
	(princ)
	(exit)
      )
    )
    (setq xcoords nil)
    (setq ycoords nil)
    (foreach item enlist
      (if (= (car item) 10)
	(progn
	  (setq	xcoords	(append xcoords (list (car (cdr item))))
		ycoords	(append ycoords (cdr (cdr item)))
	  )
	)
      )
    )
    (setq xcoord_sa (vlax-make-safearray
		      vlax-vbDouble
		      (cons 0 (- coordlength 1))
		    )
    )
    (vlax-safearray-fill xcoord_sa xcoords)
    (setq ycoord_sa (vlax-make-safearray
		      vlax-vbDouble
		      (cons 0 (- coordlength 1))
		    )
    )
    (vlax-safearray-fill ycoord_sa ycoords)
    (if line_closed
	    (vlax-invoke-method
	      net 'add_unlink xcoord_sa ycoord_sa)
      	    (vlax-invoke-method
	      net 'add_link id xcoord_sa ycoord_sa startelev endelev 1 0
	      isisland)
      )
  )
)

(defun readline (net id entity) (read-line-or-unlink net id entity nil))

(defun rgb->code ( rgblist )
	(+ (lsh (fix (nth 0 rgblist)) 16) (lsh (fix (nth 1 rgblist)) 8) (fix (nth 2 rgblist)))
)

(defun safe-sa->list (sa)
  (if (= (vlax-safearray-get-u-bound sa 1) -1)
	nil
	(vlax-safearray->list sa)
  )
)

(defun zip (xs ys / result)
  (progn
    (setq result nil)
    (while xs
      (setq result (cons (list (car xs) (car ys)) result))
      (setq xs (cdr xs))
      (setq ys (cdr ys))
    )
    (reverse result)
  )
)


    (defun dict-list  (dict /)
     (cond ((= (type dict) 'str) (dict-list (dictsearch (namedobjdict) dict)))
           ((= (type dict) 'ename) (dict-list (entget dict)))
           ((not (listp dict)) nil)
           ((eq (cdr (assoc 0 dict)) "DICTIONARY")
            (mapcar '(lambda (name) (cons (cdr name) (dict-get-xrecord (cdar dict) (cdr name))))
                    (vl-remove-if-not '(lambda (item) (= (car item) 3)) dict)))
           ((setq dict (assoc 360 dict)) (dict-list (cdr dict)))))
     
    (defun dict-prepare-data  (data / lst)
     (cond
       ((not data) '((93 . 0)))
       ((= data t) '((93 . -1)))
       ((listp data)
        (if (listp (cdr data))
          (cons (cons 91 (length data)) (apply 'append (mapcar 'dict-prepare-data data)))
          (append '((92 . 2))
                  (dict-prepare-data (car data))
                  (dict-prepare-data (cdr data)))))
       ((= (type data) 'Str)
        (setq lst (cons (cons 1 (substr data 1 255)) lst))
        (while (not (eq (setq data (substr data 256)) ""))
          (setq lst (cons (cons 3 (substr data 1 255)) lst)))
        (reverse lst))
       ((= (type data) 'Int) (list (cons 95 data)))
       ((= (type data) 'Real) (list (cons 140 data)))
       ((= (type data) 'EName) (list (cons 340 data)))))
     
    (defun dict-extract-data  (aList / data count extract-helper extract-list)
     (if (setq data (assoc 70 aList))
       (progn (setq aList (cdr (member data aList)))
              (defun extract-helper  (/ tmp len)
                (cond ((member (caar aList) '(95 140 340))
                       (setq tmp   (cdar aList)
                             aList (cdr aList)))
                      ((= (caar aList) 1)
                       (setq tmp   (cdar aList)
                             aList (cdr aList))
                       (while (= (caar aList) 3)
                         (setq tmp   (strcat tmp (cdar aList))
                               aList (cdr aList))))
                      ((= (caar aList) 91)
                       (setq len   (cdar aList)
                             aList (cdr aList))
                       (repeat len (setq tmp (cons (extract-helper) tmp)))
                       (setq tmp (reverse tmp)))
                      ((= (caar aList) 92)
                       (setq aList (cdr aList)
                             tmp   (cons (extract-helper) (extract-helper))))
                      ((= (caar aList) 93)
                       (setq tmp   (< (cdar aList) 0)
                             aList (cdr alist))))
                tmp)
              (extract-helper))))
   
(defun create-or-get-xrecord (adict name / anXrec)
  (if
    (not (setq anXrec (dictsearch adict name)))
     (progn
       ;;if name was not found then create it
       (setq anXrec (entmakex '((0 . "XRECORD")
				(100 . "AcDbXrecord")
			       )
		    )
       )
       (dictadd adict name anXrec)
       (setq anXrec (entget anXrec))
     )
  )

  ;;if it's already present then just return its entity name
  anXrec
)

(defun dict-get-data (owner name /)
  (if (setq owner (create-or-get-xrecord owner name))
    (dict-extract-data owner)
  )
)

(defun dict-put-data (owner name data / xrecord)
  (progn
    (if	(setq xrecord (cdr (car (dictsearch owner name))))
      (entdel xrecord)
    )
    (setq xrecord (create-or-get-xrecord owner name))
    (entmod (append xrecord (cons '(70 . 0) (dict-prepare-data data))))
  )
)

(defun create-or-get-dict (location name / dict)
  ;; is it present?
  (if (not (setq dict (dictsearch location name)))
    ;;if not present then create a new one
    (progn
      (setq dict (entmakex '((0 . "DICTIONARY")(100 . "AcDbDictionary"))))
      ;;if succesfully created, add it to the location
      (if dict (setq dict (dictadd location name dict)))
    )
    ;;if present then just return its entity name
    (setq dict (cdr (assoc -1 dict)))
  )
)


(defun sdnadict () (create-or-get-dict (namedobjdict) "sdna"))

(defun add-sdna-names (new_shortnames new_longnames / existing_names)
  (progn
    (setq existing_names (get-sdna-names))
    (mapcar
      (quote (lambda (long short)
	       (if (not (assoc short existing_names))
		 (setq existing_names (append existing_names (list(cons short long))))
	       )
	     )
      )
      new_longnames new_shortnames
    )
    (dict-put-data (sdnadict) "names" (flatten existing_names))
  )
)

(defun flatten (assoclist / result item)
 (progn
  (setq result nil)
  (while (not (null assoclist))
    (setq item (car assoclist)
	  result (cons (car item) result)
	  result (cons (cdr item) result)
	  assoclist (cdr assoclist)
    )
  )
  (reverse result)
 )
)

(defun get-sdna-names ()
  (unflatten (dict-get-data (sdnadict) "names"))
)

(defun sdna-names () (alert "fail"))

(defun unflatten ( flat_names / assoc_names)
 (progn
  (setq assoc_names nil)
  (while (not (null flat_names))
    (setq assoc_names (cons (cons (car flat_names) (cadr flat_names)) assoc_names)
	  flat_names (cddr flat_names)
    )
  )
  (reverse assoc_names)
 )
)

(defun long->short-name (name / assoclist)
 (progn
   ; search assoclist on values rather than keys
   (setq assoclist (get-sdna-names))
   (while (and (not (null assoclist)) (not (= name (cdr (car assoclist)))))
     (setq assoclist (cdr assoclist))
   )
   (caar assoclist)
 )
)
  
(defun short->long-name (short) (cdr (assoc short (get-sdna-names))))

(defun xdata-prepare-data (lst)
  (if (null lst)
    nil
    (progn
      (setq first    (car lst)
	    rest     (cdr lst)
	    prepared (cond ((= (type first) 'INT) (cons 1070 first))
			   ((= (type first) 'STR) (cons 1000 first))
			   ((= (type first) 'REAL) (cons 1040 first))
		     )
      )
      (cons prepared (xdata-prepare-data rest))
    )
  )
)

(defun data->dotpair (data / typ)
  (setq typ (type data))
  (cond ((= typ 'INT) (cons 1070 data))
        ((= typ 'REAL) (cons 1040 data))
	((= typ 'STR) (cons 1000 data))
  )
)

(defun xdata-extract-data (lst)
  (mapcar(quote(lambda (dottedpair) (cdr dottedpair))) lst)
)

(defun assoclist->xdata (lst / result)
  (setq result nil )
  (while (not (null lst))
    (setq item (car lst)
          result (cons (cons 1000 (car item)) (cons (data->dotpair (cdr item)) result))
	  lst (cdr lst)
    )
  )
  result
)

(defun xdata->assoclist (xdata / result lst)
  (setq result nil lst (xdata-extract-data xdata))
  (while (not (null lst))
    (setq key (car lst)
	  value (cadr lst)
	  result (cons (cons key value) result)
	  lst (cddr lst)
	  )
    )
  result
)

(defun delete-selection (selection / i)
  (progn
	(setq i 0)
    	(while (< i (sslength selection))
	 (progn
	  (entdel (ssname selection i))
	  (setq i (+ i 1))
	 )
	)
  )
)

(defun safe_iterator_next (net / success xs ys startelev endelev isisland)
	  (progn
	  	(vlax-invoke-method net 'iterator_next 'success 'xs 'ys 'startelev 'endelev 'isisland)
	        (if (= success :vlax-true)
		  (list (safe-sa->list xs) (safe-sa->list ys) startelev endelev (= isisland :vlax-true))
		  nil
		)
	  )
	)

(defun write-net (net / xs ys startelev endelev isisland nature white new_line link)
  (progn
	(vlax-invoke-method net 'reset_iterator)
	(while (setq link (safe_iterator_next net))
	  (progn
	    	(setq xs (nth 0 link) ys (nth 1 link) startelev (nth 2 link) endelev (nth 3 link) isisland (nth 4 link))
	  	(setq nature (if isisland "Traffic Island Link" "Normal"))
	        (setq white 7)
	        (setq new_line (make_polyline xs ys white))
	        (set_sdna_params new_line (list "startelev" "endelev" "nature") (list startelev endelev nature))
	  )
	)
  )
)