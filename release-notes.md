0.5.1 (2022-12-20)
==================

Changes:
- report correct status for object storage plugins
- improved french translations

Fixes:
- fix default values for AvailableLanguages to all languages

0.5.0 (2022-12-16)
==================

Changes:
- added multi-language support
- new configurations: 'AvailableLanguages', 'DefaultLanguage'
- shares: enable "stone-viewer-publication" type

Fixes:
- WO-63: instant-link only works if EnableShares is true

0.4.3 (2022-11-03)
==================

Changes:
- added 2 configurations to chose the viewer icons and the order in which they appear:
  'ViewersOrdering' and 'ViewersIcons'.  The default configuration is identical to the 0.4.1 behaviour.
- adapted orthanc-share API

0.4.2 (2022-10-28)
==================

Changes:
- implement study sharing UI for orthanc-share project (still confidential, will be presented at OrthancCon 2022)
- open MedDream with a one time token if connected with orthanc-share project
- reorganized icons for viewers to always use 'eye' and 'eye-fill' for the first 2 enabled viewers.

Fixes:
- SendTo and ApiView dropdown menu mixed up when both enabled


0.4.1 (2022-09-07)
==================

Changes:
- new `EnableSendTo` option to enable/disable the `SendTo` button

Fixes:
- SendTo and ApiView dropdown menu were mixed up



0.4.0 (2022-08-30)
==================

Changes:
- new simplified interface to query DICOM modalities and retrieve study.
  It does not allow yet browsing distant series/instances.  Still a work in progress !
- show C-Echo status of DICOM modalities.
- allow controling wildcards in text search to implement exact or partial match.
  The default remains a partial match -> if you enter a `filterValue` text in a filter,
  Orthanc will search for `*filterValue*`.  By adding a `"` at the beginning or at the end of the text, you force an exact match at the beggining or at the end of the text.
  

Fixes:
- improved redirection when running behind a reverse-proxy
- support of dynamic linking against the system-wide Orthanc framework library
- support of LSB (Linux Standard Base) compilation

Maintenance:
- upgraded all npm dependencies

0.3.3 (2022-06-09)
==================

Changes:
- added a button 'open in MedDream Viewer'
- added buttons to change log levels in settings page
- added 'transfer to peer' using the Transfers plugin

- new configurations:
  - "EnableOpenInMedDreamViewer"
  - "MedDreamViewerPublicRoot"

Fixes:
- display ReferringPhyisician, RequestingPhysician and InstitutionName in study list
- fix GDCM plugin status in settings page

0.3.2 (2022-06-01)
==================

Changes:
- showing 'searching' status and allow cancelling search

- new configuration:
  - "StudyListEmptyIfNoSearch"


0.3.1 (2022-05-31)
==================

Changes:
- added a button 'open in OHIF Viewer'
- introduced two search modes "search-as-you-type" (suitable for small DBs) or "search-button" (suitable for large DB)

- new configurations:
  - "EnableOpenInOhifViewer"
  - "OhifViewerPublicRoot"
  - "StudyListSearchMode"
  - "StudyListSearchAsYouTypeMinChars"
  - "StudyListSearchAsYouTypeDelay"


0.3.0 (2022-05-23)
==================

Changes:
- fixed 'send to peer'
- added 'send to modality'
- now displaying the transfer jobs created by 'send to peer', 'send to modality', 'send to dicom-web'

- new configuration:
  - "MaxMyJobsHistorySize"

0.2.2 (2022-05-19)
==================

Changes:
- show Orthanc "Name" in side bar
- changed color of Orthanc logo to white
- improved relative URL computation when running behind a reverse proxy
- added an 'expand' query argument with values to expand studies, series ... to use with only a few resources !
  ```
   http://localhost:8042/ui/app/#/filtered-studies?StudyInstanceUID=1.2.3&expand
   http://localhost:8042/ui/app/#/filtered-studies?StudyInstanceUID=1.2.3&expand=study
   http://localhost:8042/ui/app/#/filtered-studies?StudyInstanceUID=1.2.3&expand=series
   http://localhost:8042/ui/app/#/filtered-studies?StudyInstanceUID=1.2.3&expand=instance
  ```
- new configurations:
  - "ShowOrthancName"
  - "EnableDownloadZip"
  - "EnableDownloadDicomDir"
  - "EnableDownloadDicomFile"

0.2.1 (2022-05-13)
==================

Changes:
- new default root is now '/ui/'

Fixes:
- allow display of patient tags in upload report
- upload: avoid calling /statistics for each instance (now only once per study uploaded)

0.2.0 (2022-05-12)
==================

initial release
