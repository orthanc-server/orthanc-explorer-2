Changes:
- added a button 'open in OHIF Viewer'

- new configurations
  - "EnableOpenInOhifViewer"
  - "OhifViewerPublicRoot"


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