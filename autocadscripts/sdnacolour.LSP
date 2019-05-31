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

(defun move_to_top (doc selectionset)
  (progn
    (setq vlaobjectlist (ss->vla selectionset))
    (if	vlaobjectlist ;needed as the rest will fail on an empty list
      (progn

       (setq owner    (vla-get-OwnerID (car vlaobjectlist))
	     sortents (sortentstable doc owner)
       )
       (if (not (vl-catch-all-error-p sortents))
	(vla-MovetoTop
	  sortents
	  (vlax-make-variant
	    (vlax-safearray-fill
	      (vlax-make-safearray
		vlax-vbobject
		(cons 0 (1- (length vlaobjectlist)))
	      )
	      vlaobjectlist
	    )
	  )
	)
       )
      )
    )
  )
)

 
(defun sortentstable ( doc owner / space dict )
  (setq space (vla-ObjectIdtoObject doc owner)
	dict (vla-GetExtensionDictionary space)
  )
  (vl-catch-all-apply 'vla-AddObject (list dict "ACAD_SORTENTS" "AcDbSortentsTable"))
)

(defun ss->vla ( ss / i result )
  (setq i 0 result nil)
  (while (< i (sslength ss))
    (setq result (cons (vlax-ename->vla-object (ssname ss i)) result)
	  i (+ i 1)
    )
  )
  result
)

(vl-load-com)
(foreach item '(acad_truecolordlg)
   (vl-arx-import item)
)

(defun mod (x y / r)
  (setq r (rem x y))
  (if (>= r 0) r (+ r y))
)

(defun rgb->hsl ( rgb / r g b mn mx c h l s )
  (setq r (/ (nth 0 rgb) 255.)
        g (/ (nth 1 rgb) 255.)
        b (/ (nth 2 rgb) 255.)
        mn (min r g b)
        mx (max r g b)
	c (- mx mn)
	h (if (= c 0) 0
	    (if (= mx r) (mod (/ (- g b) c) 6)
	      (if (= mx g) (+ 2 (/ (- b r) c))
		(+ 4 (/ (- r g) c))
	      )
	    )
	  )
	h (* 60 h)
	l (/ (+ mn mx) 2)
	s (if (= c 0) 0
	    (/ c (- 1 (abs (- (* 2 l) 1))))
	  )
   )
   (list h s l)
)


(defun hsl->rgb ( hsl / h s l c x rgb m)

  (setq h (/ (nth 0 hsl) 60.)
        s (nth 1 hsl) 
        l (nth 2 hsl)
	c (* s (- 1. (abs (- (* 2. l) 1.))))
	x (* c (- 1. (abs (- (mod h 2.) 1.))))
	rgb   (if (< h 1) (list c x 0)
		(if (< h 2) (list x c 0)
		  (if (< h 3) (list 0 c x)
		    (if (< h 4) (list 0 x c)
		      (if (< h 5) (list x 0 c)
			(list c 0 x)
		      )
		    )
		  )
		)
              )
	m (- l (/ c 2.))
	rgb (mapcar '(lambda (z) (+ m z)) rgb)
	rgb (mapcar '(lambda (z) (* 255 z)) rgb)
  )
  rgb
)

(defun code->rgb (code)
  (setq code (fix code))
  (list (logand (lsh code -16) 255) (logand (lsh code -8) 255) (logand code 255))
)


(defun index->true (index)
  (progn
    (setq accm
	   (vla-getinterfaceobject
	     (vlax-get-acad-object)
	     (strcat "AutoCAD.AcCmColor." (substr (getvar 'ACADVER) 1 2))
	   )
    )
    (vla-put-colorindex accm index)
    (rgb->code (list (vla-get-red accm) (vla-get-green accm) (vla-get-blue accm)))
  )
)

(defun truecolorfromdialog (default / dlgrtn truertn)
  (setq dlgrtn (acad_truecolordlg (cons 420 default) nil))
  (if dlgrtn
    (progn
	  (setq truertn (assoc 420 dlgrtn))
	  (if truertn
	    (cdr truertn)
	    (index->true (cdr (assoc 62 dlgrtn)))
	  )
    )
    default
  )
)

(defun my_rtos (x)
  (cond
    ((>= (abs x) 10000) (rtos x 1 2)) ; scientific with 2dp = 3sf
    ((and (< (abs x) 10000) (>= (abs x) 100)) (rtos x 2 0)) ; integers only >100 = at least 3sf
    ((and (< (abs x) 100) (>= (abs x) 10)) (rtos x 2 1)) ; 1dp = 3sf
    ((and (< (abs x) 10) (>= (abs x) 1)) (rtos x 2 2)) ; 2dp = 3sf
    ((and (< (abs x) 1) (>= (abs x) 0.1)) (rtos x 2 3)) ; 3dp = 3sf
    ((and (< (abs x) 0.1) (>= (abs x) 0.01)) (rtos x 2 4)) ; 4dp = 3sf
    ((and (< (abs x) 0.01) (>= (abs x) 0.001)) (rtos x 2 5)) ; 5dp = 3sf
    ((< (abs x) 0.001) (rtos x 1 2)) ; scientific with 2dp = 3sf
))

(defun makekey (param_name colorcodes quintiles / boundaries top_point bottom_point key_height num_symbols row_height row lower upper text
		white maketext makerow)

        (setq white 16777215)

	(defun maketext (string point height colour stretch / text_shrink )
	        (setq text_shrink 0.7)
	        (setq point (list (nth 0 point) (+ (* (/ (- 1 text_shrink) 2) height) (nth 1 point)) (nth 2 point)))
		(entmake (list (cons 0 "TEXT") (cons 40 (* height text_shrink)) (cons 1 string) (cons 10 point) (cons 420 colour) (cons 41 stretch)))
	)

	(defun makerow (row top_point row_height text colour / row_x row_y row_z )
	  (setq row_x (nth 0 top_point) row_y (- (nth 1 top_point) (* row_height (1+ row))) row_z (nth 2 top_point))
	  (maketext text (list (+ row_x (* 2 row_height)) row_y row_z) row_height white 1)
	  (maketext "-" (list row_x row_y row_z) row_height colour 5)
	)

  (setq boundaries (mapcar '(lambda (x) (my_rtos x)) quintiles))
  (setq top_point (getpoint "Click to set key position (top left)\n"))
  (setq bottom_point (getpoint "Click to set key size (bottom edge)\n"))
  (setq key_height (abs (- (nth 1 bottom_point) (nth 1 top_point))))
  (setq num_symbols (length colorcodes))
  (setq row_height (/ key_height (1+ num_symbols))); extra row for title
  ;make title row
  (maketext (param_description) top_point row_height white 1)
  ;make key row 0
  (makerow 0 top_point row_height (strcat "<= " (nth 0 boundaries)) (nth 0 colorcodes))
  ;make key row 1-(n-1)
  (setq row 1)
  (while (< row (- num_symbols 1))
    (progn
      (setq lower (nth (- row 1) boundaries) upper (nth row boundaries))
      (setq text (strcat lower " - " upper))
      (makerow row top_point row_height text (nth row colorcodes))
      (setq row (1+ row))
    )
  )
  ;make key final row
  (makerow (- num_symbols 1) top_point row_height (strcat "> " (nth (- num_symbols 2) boundaries)) (nth (- num_symbols 1) colorcodes))
)

   (defun get_quintile (number quintiles / quintile answer)
     (progn
       (setq quintile 0
	     answer 0
       )
       (while (< quintile (length quintiles))
	 (progn
	   (if (> number (nth quintile quintiles))
	       (setq answer (+ quintile 1))
	   )
	   (setq quintile (+ quintile 1))
	 )
       )
       answer
     )
   )

   (defun is_colour (thing)
     (and (= (type thing) (type 1)) (>= thing 0) (< thing 16777216))
   )

   ; set default colours
   ; if they haven't been set already this session
   ; (else keep whatever is set)
    (if (not(is_colour highcolour)) (setq highcolor (rgb->code (list 255 0 0))))
    (if (not(is_colour lowcolour)) (setq lowcolor  (rgb->code(list 0 0 255))))
    (if (null gradient_type) (setq gradient_type 2))

   (defun weighted_interpolation (lowcolor highcolor weight)
     (mapcar '(lambda (low hi) (+ (* weight hi) (* (- 1 weight) low))) lowcolor highcolor)
   )

   (defun interpolate_colours (left right proportion use_hsl / result)
     (progn
       (if use_hsl (setq left (rgb->hsl left) right (rgb->hsl right)))
       (setq result (weighted_interpolation left right proportion))
       (if use_hsl (setq result (hsl->rgb result)))
     )
     result
   )

   (defun make-gradient-multicol (rgbcolorlist steps use_hsl_interpolation
				  / stepsize step outlistrgb
				    fractional_position_in_rgbcolorlist
				    left_colour right_colour proportion out_colour
				    left_colour_index right_colour_index)
     (setq 
	   stepsize (/ (- (float (length rgbcolorlist)) 1) (- (float steps) 1))
	   step 0.
	   outlistrgb nil
     )
     (while (< step steps)
       	 (setq fractional_position_in_rgbcolorlist (* step stepsize)
	       left_colour_index (fix fractional_position_in_rgbcolorlist)
	       right_colour_index (1+ left_colour_index)
	       left_colour (nth left_colour_index rgbcolorlist)
	       right_colour (if (< step (- steps 1))
			      (nth right_colour_index rgbcolorlist)
			      left_colour ; if last colour in list
			    )
	       proportion (- fractional_position_in_rgbcolorlist left_colour_index)
	       out_colour (interpolate_colours left_colour right_colour proportion use_hsl_interpolation)
	       outlistrgb (cons out_colour outlistrgb)
	       step (1+ step)
	 )
     )
     (reverse outlistrgb)
   )

   (defun gradient_is_sdna () (= gradient_type 2))
   (defun gradient_is_fade () (= gradient_type 0))
   (defun gradient_is_hsl  () (= gradient_type 1))

   (setq sdna_standard_colours (reverse (list
				 (list 255 0 0)
				 (list 255 111 0)
				 (list 255 183 0)
				 (list 255 255 0)
				 (list 210 255 105)
				 (list 145 255 180)
				 (list 0 255 255)
				 (list 56 172 255)
				 (list 54 97 255)
				 (list 0 0 255)
   )))

   (defun compute-colorcodes (/ lowcolorlocal highcolorlocal colorlists)
     (if (gradient_is_sdna)
       (setq colorlists (make-gradient-multicol sdna_standard_colours numsteps T))
       (setq lowcolorlocal (code->rgb lowcolor)
	     highcolorlocal (code->rgb highcolor)
	     use_hsl_interpolation (not (gradient_is_fade))
	     colorlists (make-gradient-multicol (list lowcolorlocal highcolorlocal) numsteps use_hsl_interpolation)
       )
     )
     (setq colorcodes (mapcar '(lambda (c) (rgb->code c)) colorlists))
    )	      

    (defun setcolourbyparam (entity	quintiles
			     / quintile quintset
			       enlist	   sdna_data
			     data	color	   new_data new_color_group
			    )
      (progn
	(setq enlist (get_entity_with_sdna_data entity))
	(setq data (data_to_display_from_entity enlist))
	(if (/= data nil)
	  (progn
		(setq quintile (get_quintile data quintiles))
		(setq color (nth quintile colorcodes))
		(setq enlist (vl-remove (assoc 62 enlist) enlist)); remove any indexed colour
		(setq enlist (vl-remove (assoc 420 enlist) enlist)); remove any true colour
		(setq new_color_group (cons 420 color))
	        (setq new_data (append enlist (list new_color_group) ))
		(entmod new_data)
		;add to quintset for drawing order change later
		(setq quintset (nth quintile selectionsets))
		(setq quintset (ssadd entity quintset))
	   )
	)
      )
    )

(defun data_to_display_from_entity (e / cp d)
  (setq d (read_sdna_param_from_enlist param_shortname e))
  (if (= d nil)
    	  nil
	  (if control_param_shortname
	    (progn
	    	(setq cp (read_sdna_param_from_enlist control_param_shortname e))
	    	(cond ((= cp 0) nil)
		      ((= cp nil) nil)
		      (t (/ d cp))
	    	)
	    )
	    d
	  )
  )
)

(defun param_description ()
  (if control_param_longname
	(strcat param_longname " / " control_param_longname)
        param_longname
  )
)

(defun recolour_network	()
  (if (or (< numsteps 2) (> numsteps 100))
    (progn
      (alert
	"Number of colour bands must be a whole number from 2-100.  Please try again."
      )
      (exit)
    )
    (progn
      (setq param_longname (nth paramnumber paramlist))
      (setq control_param_longname
	     (if (= controlparamnumber 0)
	       nil
	       (nth (- controlparamnumber 1)
		    paramlist
	       )
	     )
      )
      (setq param_shortname (long->short-name param_longname))
      (setq control_param_shortname
	     (long->short-name
	       control_param_longname
	     )
      )


      ;;loop through selection to sample field values
      (setq i 0.)
      (setq some_failed nil)
      (setq datalist nil)
      (setq len (sslength selection))
      (while (< i len)
	(progn
	  (if (= (rem i 100) 0)
	    (grtext -1
		    (strcat "Computing quantiles "
			    (rtos (* 100 (/ i len)) 2 0)
			    "%"
		    )
	    )
	  )
	  (setq
	    data (data_to_display_from_entity
		   (get_entity_with_sdna_data (ssname selection i))
		 )
	  )
	  (if (/= data nil)
	    (setq
	      datalist
	       (cons data datalist)
	    )
	    (setq some_failed T)
	  )
	  (setq i (+ i 1))
	)
      )
      (if (null datalist)
	(alert (strcat "None of the selected objects contain data: "
		       (param_description)
	       )
	)

	(progn

	  (if some_failed
	    (progn
	    (textpage)
	    (princ
	      "WARNING: some objects selected did not contain the desired data and were not coloured.\n"
	    )
	    )
	  )

	  ;;compute quintiles
	  (setq datalist (vl-sort datalist '<))
	  (setq increment (/ (float (length datalist)) numsteps))
	  (setq	i (- numsteps 1)
		quintiles nil
	  )
	  (while (> i 0)
	    (progn
	      (setq
		quintiles (cons	(nth (fix (* increment i)) datalist)
				quintiles
			  )
	      )
	      (setq i (- i 1))
	    )
	  )

	  ;;merge identical key bands
	  (setq oldquintiles quintiles)
	  (setq quintiles nil)
	  (while oldquintiles
	    (progn
	    (if (/= (car oldquintiles) (car quintiles))
	      (setq quintiles (cons (car oldquintiles) quintiles))
	    )
	    (setq oldquintiles (cdr oldquintiles))
	    )
	  )
	  (setq quintiles (reverse quintiles))
	  (setq numsteps (1+ (length quintiles)))

	  

	  ;;compute colour gradients and put in colorcodes
      	  (compute-colorcodes)

	  ;;loop through selection to colour according to quintile
	  (setq selectionsets nil)
	  (setq i 0)
	  (while (< i numsteps)
	    (setq selectionsets
		   (cons (ssadd) selectionsets)
		  i (+ i 1)
	    )
	  )
	  (setq i 0.)
	  (while (< i len)
	    (progn
	      (setcolourbyparam (ssname selection i) quintiles)
	      (setq i (+ i 1))
	      (if (= (rem i 100) 0)
		(grtext	-1
			(strcat	"Colouring network "
				(rtos (* 100 (/ i len)) 2 0)
				"%"
			)
		)
	      )
	    )
	  )
					;change drawing order
	  (setq doc (vla-get-activedocument (vlax-get-Acad-Object)))
	  (setq i 0)
	  (while (< i numsteps)
	    (progn
	      (move_to_top doc (nth i selectionsets))
	      (setq i (+ i 1))
	    )
	  )
	  (setq selectionsets nil)
	  (vla-Regen (vla-get-activedocument (vlax-get-acad-object))
		     acAllViewports
	  )
	  (princ "Net coloured according to parameter ")
	  (princ (param_description))
	  (princ "\n")
	  (princ)
	)
      )
    )
  )

)

(defun C:sdnacolour ( / param_longname param_shortname control_param_longname control_param_shortname defaultlow defaulthigh
		   colorindex	   
		   		   selection
		   i		   datalist	   increment
		   q1		   q2		   q3
		   q4		   quintiles current_entity_assoclist len truecolorindex colorcodes
		     paramlist savevars dcl_id
		  )
  (progn
    (regapp "sdna")
    (setq selection (ssget))
    (if (= selection nil) 
      (progn
	(textpage)
    	(princ "Error: can't run sDNA:\n")
	(princ "No objects selected\n")
	(exit)
      )
    )
    
    ;;loop through names dict to get available data names
    (setq longnames nil
	  assoclist (get-sdna-names))
    (while (not(null assoclist))
      (setq item (car assoclist)
	    assoclist (cdr assoclist)
	    longnames (cons (cdr item) longnames)
      )
    )

    (setq paramlist (reverse longnames))

    ;;pop up dialog
      (defun savevars ()
	  (setq lastparamnumber_s (get_tile "parameter"))
	  (setq lastcontrolparam_s (get_tile "controlparam"))
	  (setq paramnumber (atoi lastparamnumber_s))
	  (setq controlparamnumber (atoi lastcontrolparam_s))
	  (setq numsteps (atoi(get_tile "bands")))
	  (setq gradient_type (atoi(get_tile "gradient_type")))
	)
  (defun setlow () (setq lowcolor (truecolorfromdialog lowcolor)))
  (defun sethigh () (setq highcolor (truecolorfromdialog highcolor)))
  (defun docolouring () (savevars)(recolour_network))
  (setq dcl_id (load_dialog "integral.dcl"))
  (if (not (new_dialog "COLOUR" dcl_id) )
    (progn
      (setq dcl_id (load_dialog "d:\\sdna\\autocadscripts\\integral.dcl"))
      (if (not (new_dialog "COLOUR" dcl_id) ) (exit))
    )
  )
  (start_list "parameter" 3)
  (mapcar 'add_list paramlist)
  (end_list)
  (start_list "controlparam" 3)
  (add_list "None")
  (mapcar 'add_list paramlist)
  (end_list)
  (if lastparamnumber_s
  	(set_tile "parameter" lastparamnumber_s)
  )
  (if lastcontrolparam_s
        (set_tile "controlparam" lastcontrolparam_s)
        (set_tile "controlparam" "0")
  )
  (set_tile "gradient_type" (itoa gradient_type))
  (action_tile "apply" "(docolouring)")
  (action_tile "done" "(docolouring)(setq place_key_when_done nil)(done_dialog)")
  (action_tile "key" "(docolouring)(setq place_key_when_done T)(done_dialog)")
  (action_tile "cancel" "(setq place_key_when_done nil)(done_dialog)")
  (action_tile "setlow" "(setlow)")
  (action_tile "sethigh" "(sethigh)")
  (start_dialog) 
  (unload_dialog dcl_id)
  (if place_key_when_done
      (makekey (nth paramnumber paramlist) colorcodes quintiles)
  )
  (princ)


    
)
)

(defun C:sdnacolor () (C:sdnacolour))

(defun testgradient () (mapcar '(lambda (x) (progn (princ (code->rgb x)) (princ "\n") (princ))) (compute-colorcodes)))