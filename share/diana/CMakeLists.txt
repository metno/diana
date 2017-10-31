
SET (DATADIR "${CMAKE_INSTALL_FULL_DATAROOTDIR}/diana/${PVERSION}")

INSTALL (DIRECTORY products fonts images
  DESTINATION ${DATADIR}
)

INSTALL (FILES
  UK_lyn.header
  UALF_diana.header
  jason_swh.header
  printer.def
  amdar.stations
  ogckml22.xsd
  atom-author-link.xsd
  xAL.xsd
  iptab_metar.txt
  iptab_synop.txt
  itab_metar.txt
  itab_synop.txt

  DESTINATION ${DATADIR}
)
