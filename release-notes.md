Changes:
- new simplified interface to query DICOM modalities and retrieve study.
  It does not allow yet browsing distant series/instances.
- show C-Echo status of DICOM modalities.

Fixes:
- improved redirection when running behind a reverse-proxy
- support of dynamic linking against the system-wide Orthanc framework library
- support of LSB (Linux Standard Base) compilation


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
