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
    

(defun C:sdnaprepare ( / get_params_with_dialog setcolourbyproblem white pink red blue green fundamental_indices)
  (progn

    (setq white (rgb->code (list 255 255 255)) pink (rgb->code (list 255 150 150)) red (rgb->code (list 255 0 0))
	  blue (rgb->code (list 50 50 255)) green (rgb->code (list 0 255 0)))
    
    (defun setcolourbyproblem (entity is_split is_ti is_isolated is_duplicate
			     /		enlist	   color   new_data new_color_group)
      (progn
	(setq enlist (get_entity_with_sdna_data entity))
	(setq enlist (vl-remove (assoc 62 enlist) enlist)); remove any indexed colour
	(setq enlist (vl-remove (assoc 420 enlist) enlist)); remove any true colour

	(setq color white)
	(if is_split (setq color pink))
	(if is_ti (setq color red))
	(if is_isolated (setq color blue))
	(if is_duplicate (setq color green))
	
	(setq new_color_group (cons 420 color))
        (setq new_data (append enlist (list new_color_group) ))
	(entmod new_data)
      )
    )
    
    (defun get_params_with_dialog(/ savevars dcl_id ddiag)
  (defun savevars ()
	  (setq splitlinkaction (get_tile "split_links"))
	  (setq trafficislandaction (get_tile "traffic_islands"))
	  (setq duplicateaction (get_tile "duplicate_links"))
	  (setq isolatedaction (get_tile "isolated_systems"))
	)
  (setq dcl_id (load_dialog "integral.dcl"))
  (if (not (new_dialog "PREPARE" dcl_id) )
    (progn
    	(setq dcl_id (load_dialog "d:\\sdna\\autocadscripts\\integral.dcl"))
        (if (not (new_dialog "PREPARE" dcl_id) ) (exit))
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
  )
)
    

    ;;execution starts here
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

    (princ "sDNA Prepare running, reading network...\n")
    (setq i 0)
    (while (< i (sslength selection))
      (progn
	(readline net i (ssname selection i) )
	(setq i (+ i 1))
      )
    )


    (setq splitlinklist nil trafficislandlist nil isolatedlist nil duplicatelist nil)
    
    (if (= trafficislandaction "tid")
      (progn
	(vlax-invoke-method net 'get_traffic_islands 'traffic_island_ids) 
	(setq trafficislandlist (safe-sa->list traffic_island_ids))
	(if trafficislandlist (progn (princ "Traffic islands detected; coloured in red\n")))
      )
    )
    (if (= duplicateaction "dld")
      (progn
	(vlax-invoke-method net 'get_duplicates 'duplicate_ids 'original_ids)
	(setq duplicatelist (safe-sa->list duplicate_ids))
	(setq originallist (safe-sa->list original_ids))
	(setq duplicatelist (append duplicatelist originallist))
	(if duplicatelist (progn (princ "Duplicate links detected; coloured in green\n")))
      )
    )
    (if (= isolatedaction "isd")
      (progn
	(vlax-invoke-method net 'get_subsystems 'message 'isolated_ids)
	(setq isolatedlist (safe-sa->list isolated_ids))
	(if isolatedlist (progn (princ "Isolated subsystems detected; coloured in blue\n")))
      )
    )
    (if (= splitlinkaction "sld")
      (progn
	(vlax-invoke-method net 'get_split_links 'split_link_ids)
	(setq splitlinklist (safe-sa->list split_link_ids))
	(if splitlinklist (progn (princ "Split links detected; coloured in pink\n")))
      )
    )

    (if (and (= splitlinklist nil) (= trafficislandlist nil) (= isolatedlist nil) (= duplicatelist nil))
      (progn
	;; nothing was detected that needs displaying to user,
	;; so we can go ahead and repair
    
	    (if (= trafficislandaction "tir")
	      (progn
		(princ "Fixing traffic islands\n")
		(vlax-invoke-method net 'fix_traffic_islands) 
	      )
	    )
	    (if (= duplicateaction "dlr")
	      (progn
		(princ "Erasing duplicates\n")
		(vlax-invoke-method net 'fix_duplicates) 
	      )
	    )
	    (if (= isolatedaction "isr")
	      (progn
		(princ "Erasing isolated systems\n")
		(vlax-invoke-method net 'fix_subsystems) 
	      )
	    )
	    (if (= splitlinkaction "slr")
	      (progn
		(princ "Fixing split links\n")
		(vlax-invoke-method net 'fix_split_links)
	      )
	    )

	;; then delete selection and replace with new net
	(princ "Writing new net...\n")
	(delete-selection selection)
	(write-net net)
	(princ "...done.\n")
	(princ)
      )
      (progn
	;; ELSE something was detected so we display it
	     (setq splitlinklist (vl-sort splitlinklist '<))
	     (setq trafficislandlist (vl-sort trafficislandlist '<))
	     (setq isolatedlist (vl-sort isolatedlist '<))
	     (setq duplicatelist (vl-sort duplicatelist '<))
	     
	     (setq i 0.)
	     (while (< i (sslength selection))
	       (progn
		 ;; pull out detail of this link from problem lists - NB relies on correct ordering of selection and lists!
		 (setq is_split nil is_ti nil is_isolated nil is_duplicate nil)
		 (if (= (car splitlinklist) i) (setq is_split T splitlinklist (cdr splitlinklist)))
		 (if (= (car trafficislandlist) i) (setq is_ti T trafficislandlist (cdr trafficislandlist)))
		 (if (= (car isolatedlist) i) (setq is_isolated T isolatedlist (cdr isolatedlist)))
		 (if (= (car duplicatelist) i) (setq is_duplicate T duplicatelist (cdr duplicatelist)))
		 (setcolourbyproblem (ssname selection i) is_split is_ti is_isolated is_duplicate)
		 (setq i (+ i 1))
		 (if (= (rem i 100) 0) (grtext -1 (strcat "Displaying " (rtos (* 100 (/ i (sslength selection))) 2 0) "%")))
	       )
	     )

	     (princ "No repairs made\n") (princ)
      )
    )
	
    (vlax-release-object net)
    (vlax-release-object window)
    (princ)
  )
)