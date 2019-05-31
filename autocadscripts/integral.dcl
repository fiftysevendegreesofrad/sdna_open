INTEGRAL : dialog {
    label = "sDNA Integral";
  : column {
	: boxed_column {
	  label = "Analysis type";
	  	: radio_row  {
		  key = "analysis_type";
		  : radio_button {
		    label = "Angular";
		    key = "ANGULAR";
		    value = "1";
		  } 
		  : radio_button {
		    label = "Euclidean";
		    key = "EUCLIDEAN";
		  }
		  : radio_button {
		    label = "Custom";
		    key = "CUSTOM";
		  }
		  : radio_button {
		    label = "Hybrid";
		    key = "HYBRID";
		  }		  
		}
		: toggle {
		  key = "length_weighted";
		  label = "Weight by link length";
		  value = "0";
		  }
	   }

    : boxed_column {
      label = "Locality of analysis";
      	    : text {
	      label = "Enter list of network Euclidean radii separated by commas (use n for global)";
	      }
	    : edit_box {
	      key = "radii";
	      label = "Radius list:";
	      edit_width = 50;
	      value = "400,800,1200";
	      }
	    : toggle {
              key = "cont_space";
              label = "Use continuous space";
              value = "0";
              }
      }
    : boxed_column {
      label = "Advanced";
      	    : edit_box {
	      key = "advanced";
	      label = "Extra options:";
	      edit_width = 50;
	      value = "";
	      }
      }
    : row {
	    : button {
	      key = "accept";
	      label = " Analyze ";
	      is_default = true;
	    }

	    : button {
	      key = "cancel";
	      label = " Cancel ";
	      is_default = false;
	      is_cancel = true;
	    }
     }

  }
  

}

COLOUR : dialog {
    label = "sDNA Colour";
  : column {
   : row {
    : boxed_row {
       label = " Parameter to display ";
	: list_box {
                key = "parameter";
                height = 35;
                width = 40;
                multiple_select = false;
                fixed_width_font = false;
                value = "";
        }
    }
    : boxed_row {
       label = " Control parameter ";
        : list_box {
                key = "controlparam";
                height = 35;
                width = 40;
                multiple_select = false;
                fixed_width_font = false;
                value = "";
        }
     }
    }

    : row {
    : boxed_column {
        label = " Gradient ";
        : edit_box {
	  key = "bands";
	  label = "Number of colour bands (2-100):";
	  edit_width = 3;
	  value = "10";
	  }
        : radio_row  {
		  key = "gradient_type";
		  : radio_button {
		    label = "sDNA standard";
		    key = "2";
		    value = "1";
		  }
		  : radio_button {
		    label = "Colour fade";
		    key = "0";
		  } 
		  : radio_button {
		    label = "HSL cycle";
		    key = "1";
		  }
	}
	: text {
		value = "End colours (for colour fade and HSL cycles only):";
	}
        : row {
        : button {
            label = "Low colour";
            key = "setlow";
            is_default = false;
          }
        : button {
            label = "High colour";
            key = "sethigh";
            is_default = false;
          }
    }

    }
    : boxed_column {
      label = "Actions";
    : button {
      key = "apply";
      label = " Apply colours ";
      is_default = false;
      }
    
    : button {
      key = "done";
      label = " Apply colours and finish ";
      is_default = false;
      }

    : button {
      key = "key";
      label = " Apply colours, finish and place key ";
      is_default = true;
    }

    : button {
      key = "cancel";
      label = " Finish ";
      is_default = false;
      is_cancel = true;
    }
    }
    }

  }

}

PREPARE : dialog {
    label = "sDNA Prepare";
  : column {
  	: text { value = "Split Links"; }
	: radio_row  {
		  key = "split_links";
		  : radio_button {
		    label = "Ignore";
		    key = "sli";
		  } 
		  : radio_button {
		    label = "Detect";
		    key = "sld";
		  }
		  : radio_button {
		    label = "Repair";
		    key = "slr";
		    value = "1";
		  } 
		}
  	: text { value = "Traffic Islands"; }		
	: radio_row  {
		  key = "traffic_islands";
		  : radio_button {
		    label = "Ignore";
		    key = "tii";
		  } 
		  : radio_button {
		    label = "Detect";
		    key = "tid";
		  }
		  : radio_button {
		    label = "Repair";
		    key = "tir";
		    value = "1";
		  } 
		}
  	: text { value = "Duplicate Links"; }		
	: radio_row  {
		  key = "duplicate_links";
		  : radio_button {
		    label = "Ignore";
		    key = "dli";
		  } 
		  : radio_button {
		    label = "Detect";
		    key = "dld";
		  }
		  : radio_button {
		    label = "Repair";
		    key = "dlr";
		    value = "1";
		  } 
		}
  	: text { value = "Isolated Systems"; }		
	: radio_row  {
		  key = "isolated_systems";
		  : radio_button {
		    label = "Ignore";
		    key = "isi";
		  } 
		  : radio_button {
		    label = "Detect";
		    key = "isd";
		  }
		  : radio_button {
		    label = "Repair";
		    key = "isr";
		    value = "1";
		  } 
		}



    : row {
	    : button {
	      key = "accept";
	      label = " Prepare ";
	      is_default = true;
	    }

	    : button {
	      key = "cancel";
	      label = " Cancel ";
	      is_default = false;
	      is_cancel = true;
	    }
     }

  }
  

}

IMPORTDRAWING : dialog {
    label = "sDNA Import from drawing";
  : column {
:boxed_column {label="Import Drawing will convert line drawn networks to sDNA networks."; width=60;
:text {value="In a line drawn network, connectivity is assumed wherever lines cross, unless there is an"; width=60;}
:text {value="unlink (i.e. a bridge, tunnel etc).  Thus, all lines will be broken at their intersections"; width=60;}
:text {value="ready for sDNA processing.  To specify an unlink, use a rectangle or polygon that covers the"; width=60;}
:text {value="relevant intersections: no line breaking will occur within unlink areas."; width=60;}
:text {value=""; width=60;}
:text {value="Import Drawing also gives you the option to remove stubs shorter than a certain length."; width=60;}
:text {value="Stubs are short dead ends left after the line breaking process.  This is highly recommended,"; width=60;}
:text {value="as sDNA will otherwise process stubs as complete links, which will introduce errors into the"; width=60;}
:text {value="result."; width=60;}
:text {value=""; width=60;}
}
    
  : edit_box {
	  key = "stubs";
	  label = "Remove stubs shorter than (units of display)";
	  edit_width = 3;
	  value = "1";
	  }
: row {
	    : button {
	      key = "accept";
	      label = " Convert ";
	      is_default = true;
	    }

	    : button {
	      key = "cancel";
	      label = " Cancel ";
	      is_default = false;
	      is_cancel = true;
	    }
     }
}
}
