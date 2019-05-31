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
    
(defun C:sdnaloaditnf (filename / Doc fc netmember roadlink fid nature polyline coords
		            dn1 dn1o dn1gs dn2 dn2o dn2gs startelev endelev coords
		            isisland iselevated green red yellow purple white colour linedef
		            elev2int get_attribute_with_name find_sibling_with_name
		            find_child_with_name split num_nodes )

	(defun split (str delim / ptr lst)
	  (while (setq ptr (vl-string-search delim str))
	    (setq lst (cons (substr str 1 ptr) lst))
	    (setq str (substr str (+ ptr 2)))
	  )
	  (reverse (cons str lst))
	)


	(defun find_child_with_name (Node Name / child)
	  (setq child (vlax-get Node "firstChild"))
	  (while (and (/= child nil) (/= (vlax-get child "nodeName") Name))
	      (setq child (vlax-get child "nextSibling"))
	  )
	  child
	)

	(defun find_sibling_with_name (Node Name / sibling)
	  (setq sibling (vlax-get Node "nextSibling"))
	  (while (and (/= child nil) (/= (vlax-get sibling "nodeName") Name))
	      (setq sibling (vlax-get sibling "nextSibling"))
	  )
	  sibling
	)

	(defun get_attribute_with_name (Node Name / attributes attribute)
	  (setq attributes (vlax-get Node "attributes"))
	  (while (and (setq attribute (vlax-invoke attributes "nextNode"))
	              (/= (vlax-get attribute "nodeName") Name)
		 )
		 ()
	  )
	  (if (/= nil attribute) (vlax-get attribute "nodeValue") 0)   
	)
		    

	(defun elev2int (elev)
	  (if (= elev 0) 0 (atoi elev))
	)

  	(prompt "\rLoading ITN, please Wait...")
  	(vl-load-com)
  	(vlax-invoke-method (vlax-get-Acad-Object) 'Update)
	(regapp "sdna")
	(setq Doc (vlax-create-object "MSXML.DOMDocument"))
	(vlax-put Doc "async" 0)
	(vlax-invoke Doc "load" filename)

	(setq fc (find_child_with_name Doc "osgb:FeatureCollection"))
	(setq netmember (find_child_with_name fc "osgb:networkMember"))
        (setq num_nodes 0)
        (grtext -1 "Loading GML...")
        (while (/= netmember nil)
	  (progn
	    (if	(setq roadlink (find_child_with_name netmember "osgb:RoadLink"))
	      (progn
		(if (= (rem num_nodes 100) 0) (grtext -1 (strcat "Loading GML, " (rtos num_nodes) " links processed")))
		(setq num_nodes (1+ num_nodes))
		(setq fid (get_attribute_with_name roadlink "fid"))
		(setq nature (find_child_with_name roadlink "osgb:natureOfRoad"))
		(setq nature (vlax-get (vlax-get nature "firstChild") "nodeValue"))
		(setq polyline (find_child_with_name roadlink "osgb:polyline"))
		(setq linestring (find_child_with_name polyline "gml:LineString"))
		(setq coords (find_child_with_name linestring "gml:coordinates"))
		(setq coords (vlax-get (vlax-get coords "firstChild") "nodeValue"))
		(setq dn1 (find_child_with_name roadlink "osgb:directedNode"))
		(setq dn1o (get_attribute_with_name dn1 "orientation"))
		(setq dn1gs (get_attribute_with_name dn1 "gradeSeparation"))
		(setq dn2 (find_sibling_with_name dn1 "osgb:directedNode"))
		(setq dn2o (get_attribute_with_name dn2 "orientation"))
		(setq dn2gs (get_attribute_with_name dn2 "gradeSeparation"))
		(if (= dn1o dn2o) (*error* "Two directed nodes the same"))
		(setq startelev (if (= dn1o "-") dn1gs dn2gs))
		(setq endelev (if (= dn1o "+") dn1gs dn2gs))
		(setq startelev (elev2int startelev))
		(setq endelev (elev2int endelev))
		(setq coords (split coords " "))
		(setq coords
		       (mapcar
				(quote (lambda (x)
				  	(mapcar (quote (lambda (y) (atof y)))
						(split x ",")
					)
				))
				coords
			)
		)
		(setq xs (mapcar(quote(lambda (coord) (car coord))) coords))
		(setq ys (mapcar(quote(lambda (coord) (cadr coord))) coords))
		(setq isisland (or (= nature "Traffic Island Link") (= nature "Traffic Island Link At Junction")))
		(setq iselevated (or (/= startelev 0) (/= endelev 0)))
		(setq green 3 red 1 yellow 2 white 7 purple 6)
		(setq colour (if iselevated purple
			                  (if isisland red white)))
		(setq new_line (make_polyline xs ys colour))
		(set_sdna_params new_line (list "startelev" "endelev" "nature") (list startelev endelev nature))
	      )
	    )
	    (setq netmember (vlax-get netmember "nextSibling"))
	  )
	)
  (command "zoom" "e")
  (prompt "\r...ITN loaded.")
  (princ)
)

(defun C:sdnaloaditn () 
	(C:sdnaloaditnf (getfiled "Select GML file" "" "gml" 4))
)