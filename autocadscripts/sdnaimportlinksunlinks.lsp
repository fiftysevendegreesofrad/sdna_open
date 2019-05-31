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
    
;temporary funcs to help colour unlinks until this is supported in backend
(defun get-points (entity / enlist coordlength coords item x y)
  (progn
    (setq enlist (get_entity_with_sdna_data entity))
    (setq coordlength (cdr (assoc 90 enlist)))
    (setq coords nil)
    (foreach item enlist
      (if (= (car item) 10)
	(progn
	  (setq	x (car (cdr item))
		y (cadr (cdr item))
		coords (cons (list x y) coords)
	  )
	)
      )
    )
  )
  coords
)

(defun colour-poly-red (point_list / ctr sset item check)
  (progn
    (setq sset (apply 'ssget (list "_CP" point_list)))
    (setq ctr 0)
    (repeat (sslength sset)
      (setq item (ssname sset ctr))
      (setq item (vlax-ename->vla-object item))
      (setq check (vlax-property-available-p item "Color" T))
      (if check
	(vlax-put-property item 'Color 1)
      )					
      (setq ctr (1+ ctr))
    )			
  )
)

(defun colour-polys-red (poly_list)
  (mapcar (quote (lambda (x) (colour-poly-red x))) poly_list)
)

;the actual conversion function
(defun C:sdnaconvertunlinks ( / unlink_list)
  (progn
    (setq selection (ssget))
    (textpage)
    (if (= selection nil) 
      (progn
	(alert "sDNA Convert Unlinks\n\nThis tool converts drawings from the Lines/Unlinks format to the sDNA network format.\n\nIn the Lines/Unlinks format, polylines represent one or more links, polygons represent unlinks, and intersection implies connectivity except in an unlink.\n\nIn sDNA format, polylines represent single links; intersection implies an unlink and link endpoints must match for them to be connected.\n\nTo use this tool, first create a drawing containing your links as polylines, and your unlinks as polygons.  Then rerun the tool to convert the drawing.")
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
	      "One or more of the objects selected is not a 2d polyline/polygon\n"
	    )
	    (exit)
	  )
	)
	(setq i (+ i 1))
      )
    )

    (vl-load-com)
    (regapp "sdna")
    (setq window (vlax-create-object "sdnacomwrapper.SDNAWindow"))
    (check-sdnaprog-return window)
    (setq net (vlax-create-object "sdnacomwrapper.SpatialNet"))
    (check-sdna-return net)
    

    (vlax-invoke-method net 'configure window)

    (princ "sDNA Import Links Unlinks running, reading network...\n")
    (setq i 0)
    (setq unlink_list nil)
    (while (< i (sslength selection))
      (progn
	(read-line-or-unlink net i (ssname selection i) T)
	(if (line-closed (ssname selection i))
	  (setq unlink_list (cons (get-points (ssname selection i)) unlink_list))
	)
	(setq i (+ i 1))
      )
    )

    (vlax-invoke-method net 'import_from_link_unlink_format)
    
	;; then delete selection and replace with new net
	(princ "Writing new net...\n")
	(delete-selection selection)
	(write-net net)
        (colour-polys-red unlink_list)
	(princ "...done.\n")
	(princ)
      
	
    (vlax-release-object net)
    (vlax-release-object window)
    (princ)
  )
)