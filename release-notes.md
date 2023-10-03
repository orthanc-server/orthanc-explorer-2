1.1.2 (2023-10-03)
==================

Fixes:
- new labels now appears/disappears on the side bar when they are created/deleted from a study
- study labels are listed correctly when reoopening a study


1.1.1 (2023-09-18)
==================

Changes:
- added German translations
- show username in side bar

Fixes:
- internal: fixed a typo in tools/find request
- fix UI loading when user is not authorized to access to DicomWEB servers
- #30: preserve double spaces in tag values.
- fix labels filtering

1.1.0 (2023-08-16)
==================

Fixes:
- fixed an XSS vulnerability (many thanks to Abebe Hailu from kth.se)

1.0.3 (2023-07-14)
==================

Changes:
- added support for opening the WSI viewer on series

1.0.2 (2023-06-27)
=================

Fixes:
- fix ModalitiesInStudy filtering
- fix links to uploaded studies


1.0.1 (2023-06-21)
=================

Changes:
- added support for OHIF plugin:
  - new default value for the 'OhifViewer3PublicRoot' configuration: '/ohif/'
  - now displaying 3 buttons to open OHIF in basic mode, Volume Rendering and TMTV modes
  - 2 more viewers can be listed in 'ViewersIcons' and in 'ViewersOrdering': "ohif-vr" & "ohif-tmtv"
- added support for labels
  - new 'UiOptions.EnableEditLabels' configuration
- added support for VolView 1.1 plugin
- 'StudyListColumns' can now contain any DICOM Tag that is stored as a MainDicomTag or ExtraMainDicomTag
- added Ukrainian and Chinese translations thanks to Stephen D. Scotti, 'Franklin' and Juriy Cherednichock

Fixes:
- use auth token for /preview if required


0.9.3 (2023-05-15)
==================

Changes:
- allow opening MedDream viewer on multiple studies
- new italian translations thanks to Stefano Feroldi
- new georgian translations thanks to Yomarbuzz

Fixes:
- improved integration with OHIF v3 and keycloak


0.9.2 (2023-04-17)
==================

Changes:
- new configuration 'StudyListContentIfNoSearch' to replace 'StudyListEmptyIfNoSearch'.  Allowed values are
  "empty", "most-recents" (default)
- configuration 'StudyListEmptyIfNoSearch' is now deprecated.  You should now use "StudyListContentIfNoSearch": "empty".
- now reporting the status of Multitenant DICOM plugin in the settings page
- added button "Open Orthanc Explorer 2" in Orthanc Explorer
- new configuration 'EnableOpenInOhifViewer3' and 'OhifViewer3PublicRoot'

Fixes:
- In the settings page, the status of the python plugin was not correct.


0.8.2 (2023-03-27)
==================

Fixes:
- If OE2 configuration did not include "UiOptions" section, the UI failed to run correctly.

Changes:
- Added an icon to open a study or a series in VolView
- In the settings page, do not show buttons to open plugin UI if user-permissions are enabled, these UI
  wouldn't work anyway since they do not support tokens.
  
0.8.1 (2023-03-24)
==================

Fixes:
- Since the introduction of multiple selections, is was not possible to open the StoneViewer on a single study
- Auth tokens were not refreshed correctly.

0.8.0 (2023-03-22)
==================

Changes:
- Allow actions on multiple studies:
  - open StoneViewer
  - send to
  - delete
- Refactored the Study List headers to show bulk action buttons

Fixes:
- #9 EnableAnonimization spelled wrong in configuration file: you should now use UiOptions.EnableAnonymization instead.  Both spellings
  are currently accepted but only the new spelling might be accepted in future versions.

0.7.0 (2023-03-17)
==================

Changes:
- Introduced a Date Picker for the StudyDate and PatientBirthDate
- Introduced user-permissions and authorization tokens linked to the authorization plugin and new auth-service API
- Configuration: new "Tokens" section
- new "landing" page at /ui/app/token-landing.html?token=... to validate tokens and display a user friendly message if the token
  is invalid.  Redirects to the viewer if the token is valid.
- new "landing" page at /ui/app/retrieve-and-view.html?StudyInstanceUID=1.2.3....&modality=pacs&viewer=stone-viewer to open the viewer
  on a study that might already be stored in Orthanc or, if not, that can be fetched from a DICOM modality.

BREAKING CHANGES:
- Shares: removed anonymized shares
- Shares: is now using the authorization plugin to generate tokens.  This requires the new auth-service API
- Shares: removed "Shares" configuration section, part of it has been moved to the "Tokens" section


0.6.0 (2023-02-03)
==================

Changes:
- allow modification of series and studies
- allow anonymization of series and studies
- new configuration 'Modifications'
- new configuration 'TranslateDicomTags'
- new configurations 'StudyMainDicomTags', 'PatientMainDicomTags' to define the list of tags to display in the study details
- new link to open all studies from the same patient

Note: the 'Modify' and 'Anonymization' dialog are only available in English in this version.

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
