#!/bin/sh

DESTINATION=./Attribute-Reference.md

cat << "EOF" > ${DESTINATION}
### Attribute description
Attribute descriptions are in the following format: section.attributename, _valuetype_, short attribute description

Here follows a list of defined attribute valuetypes used in the reference documentation:

_bool_, a string value of _true_ or _false_

_integer_, a signed integer value

_string_, a string value

_xyz_id_, specifies an id reference of type xyz

_duration_, durations can be specified in frames, seconds and milliseconds with integer suffix s, ms, f eg. 20s, 20000ms

_[val1 : val2]_, this describes a list of fixed string values for attribute

_[val1 (xyz_id), val2 (duration), val3 (integer)]_, comma seperated values ex: 43,4s,200

_[val1 (string), ...]_, comma separated list of _n_ values

_float_, a floating point number

_alignment_, can be any of: topleft, top, topright, left, center, right, bottomleft, bottom, bottomright

_label_, specified as: "hidden" or as: x (integer), y (integer), justify (string), valign (string), style (string); justify can be [left:center:right]; valign can be [top:center:bottom]; style is any font style defined in engine/font_settings.txt
EOF

./extract_xml.sh  | xsltproc wiki.xslt -  |awk '{if (NR!=1) {print}}' >> ${DESTINATION}

echo "Created ${DESTINATION}"
echo "Now move it to the wiki."
