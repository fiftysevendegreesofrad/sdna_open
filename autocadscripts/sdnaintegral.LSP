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
    
    

(defun C:sdnaintegral (   )
  (progn
    



    (defun get_params_with_dialog(/ savevars dcl_id ddiag split)
      (defun split (string)
  (progn
    (setq result nil)
    (setq begin 1)
    (while (< begin (1+ (strLen string)))
      (progn
	(setq substrlen 1)
	(while (and (/= (substr string (+ begin substrlen) 1) ",")
		    (< (+ begin substrlen) (1+ (strLen string)))
	       )
	  (setq substrlen (1+ substrlen))
	)
      )
      (setq result (cons (substr string begin substrlen) result))
      (setq begin (+ begin substrlen 1))
    )
    (reverse (mapcar  '(lambda (x) (if (= x "n") -1. (atof x))) result))
   )
)
  (defun savevars ()
	  (setq analtype_s (get_tile "analysis_type"))
	  (setq lenwt_s (get_tile "length_weighted"))
	  (setq radii_s (get_tile "radii"))
	  (setq contspace_s (get_tile "cont_space"))
          (setq advanced (get_tile "advanced"))
	)
  (setq dcl_id (load_dialog "integral.dcl"))
  (if (not (new_dialog "INTEGRAL" dcl_id) )
    (progn
    	(setq dcl_id (load_dialog "d:\\sdna\\autocadscripts\\integral.dcl"))
        (if (not (new_dialog "INTEGRAL" dcl_id) ) (exit))
    )
  )
  (action_tile "cancel" "(setq ddiag nil)(done_dialog)")
  (action_tile "accept" "(setq ddiag T)(savevars)(done_dialog)")
  (start_dialog) 
  (unload_dialog dcl_id)
  (if (= ddiag nil)
    (progn
     (princ "Cancelled.\n")
     (exit)
    )
    (progn
	(setq analtype analtype_s)
        (setq lenwt (atoi lenwt_s))
        (setq contspace contspace_s)
        (setq radii radii_s)
    )
  )
)
    
    (defun writeline (calc	      id	      entity
		      param_names      /	
		      output_params   
		     )
      (progn
	(vlax-invoke-method calc 'get_all_outputs id 'output_params)
	(set_sdna_params entity param_names (vlax-safearray->list output_params))
      )
    )

;execution starts here
    (setq selection (ssget))
    (textpage)
    (if (= selection nil) 
      (progn
	(princ "Error: can't run sDNA:\n")
	(princ "No objects selected\n")
	(exit)
      )
    )
    (setq i 0)
    (while (< i (sslength selection))
      (progn
	(if (/=	"LWPOLYLINE"
		(cdr (assoc 0 (entget (ssname selection i))))
	    )
	  (progn
	    (princ "Error: can't run sDNA:\n")
	    (princ
	      "One or more of the objects selected is not a 2d polyline\n"
	    )
	    (exit)
	  )
	)
	(setq i (+ i 1))
      )
    )

    (get_params_with_dialog)
    
    (vl-load-com)
    (regapp "sdna")
    (setq window (vlax-create-object "sdnacomwrapper.SDNAWindow"))
    (check-sdnaprog-return window)
    (setq net (vlax-create-object "sdnacomwrapper.SpatialNet"))
    (check-sdna-return net)
    (vlax-invoke-method net 'configure window)
    (setq calculation (vlax-create-object "sdnacomwrapper.Calculation"))
    (check-sdna-return calculation)

    (setq config (strcat "radii=" radii ";metric=" analtype_s ";cont=" contspace_s ";" advanced))
    (if (= 1 lenwt) (setq config (strcat config ";weight_type=length")))
    
    (setq success (vl-catch-all-apply 'vlax-invoke-method
      			              (list calculation 'configure (vlax-make-variant config) net window)
    ))
    (if (vl-catch-all-error-p success)
      (progn
	(princ "Error in calculation config\n")
	(princ "Check sDNA window for details\n")
	(princ)
	(vlax-release-object net)
	(vlax-release-object calculation)
	(exit)
      )
    )

    (vlax-invoke-method calculation 'get_output_names 'longnames)
    (vlax-invoke-method calculation 'get_output_shortnames 'shortnames)
    (setq longnames (vlax-safearray->list longnames))
    (setq shortnames (vlax-safearray->list shortnames))
    (add-sdna-names shortnames longnames)
    
    (setq i 0)
    (while (< i (sslength selection))
      (progn
	(readline net i (ssname selection i) )
	(setq i (+ i 1))
      )
    )

    (setq success (vl-catch-all-apply 'vlax-invoke-method (list calculation 'run)))
    (if (not (vl-catch-all-error-p success))
      (progn
	    (setq i 0.)
	    (while (< i (sslength selection))
	      (progn
		(writeline calculation i (ssname selection i) shortnames)
		(if (= (rem i 100) 0) (grtext -1 (strcat "Saving data to network " (rtos (* 100 (/ i (sslength selection))) 2 0) "%")))
		(setq i (+ i 1))
	      )
	    )
	    (grtext -1 "Done")
	    (princ
	      "sDNA integral completed successfully; use the sdnacolour command to display results.\n"
	    )
	    (princ)
      )
      (progn
	   (princ "sDNA integral calculation failed.\n")
	   (princ)
      )
    ) 

    (vlax-release-object net)
    (vlax-release-object calculation)
    (vlax-release-object window)
    
  )
)

