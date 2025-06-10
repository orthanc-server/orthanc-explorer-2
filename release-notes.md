Pending changes (not released yet)
==================================

Changes:
  - New configuration "Tokens.LandingOptions": when opening a share link, this option enables
    the display of a landing page with, e.g., a download button, a button to open the study in
    the viewer and, optionally, a custom button.
  - added Portuguese translations thanks to Rafael Souza.


1.8.3 (2025-04-07)
==================

Changes:
  - New "multi label" search field.  Since it consumes space in the study list header,
    it is disabled by default and can be enabled with the new configuration "UiOptions.EnableMultiLabelsSearch"
  - Custom buttons: new keyword: `"{study-resource-token/download-instant-link}"`
    e.g: `"Url": "../../series/{UUID}/archive?token={study-resource-token/download-instant-link}"`
  - added Vietnamese translations thanks to Khanhhcmut.
  - added Arabic translations thanks to Ibrahim Mohamed.

Fixes:
  - Fix navigation issues between settings and local/remote studies list.
  - Fix permission edition wrt "All current and future labels"


1.8.2 (2025-04-07)
==================

Changes:
=======
  - New configuration "UiOptions.DefaultOrdering" and new default value for 
    "StudyListContentIfNoSearch": "default-ordering".
  - Now showing the status of the Housekeeper plugin in the "System Info"
  - Added Romanian translations.

Fixes:
  - Fix [#72](https://github.com/orthanc-server/orthanc-explorer-2/issues/72): 
    Can't modify StudyDescription if PatientName and PatientBirthDate are empty and if the patient has multiple studies

1.8.1 (2025-04-01)
==================

Changes:
  - Retrieve from remote DICOM modalities: Use the "LocalAet" defined at modality level as the TargetAet.
  - New configuration "AdvancedOptions.DownloadInstantLinksUseRestApiWithAuthHeaders".
    See the documentation of the configuration for all details.

Fixes:
  - Fix [#77](https://github.com/orthanc-server/orthanc-explorer-2/issues/77): Invalid default configuration file.
  - Retrieve from remote DICOM modalities: Use the "RetrieveMethod" defined at modality level.

  

1.8.0 (2025-02-27)
==================

Changes:
  - New configuration "UiOptions.CustomButtons" to enable custom buttons on resources.
    See the documentation of the configuration for all details.
  - New configuration "UiOptions.AutoAddWildcardsToSearchFields" (default = true)
    to enable/disable automatic adding of wildcards before and after the value entered in the UI search fields
    for textual values.
  - New configurations to customize the filenames of the downloads:
    "DownloadStudyFileNameTemplate", "DownloadSeriesFileNameTemplate" and "DownloadInstanceFileNameTemplate".
    The last configuration will only work with an Orthanc 1.12.7+.
  - New configuration "Tokens.RequiredForLinks" can be set to false
    when using HTTP Basic authentication together with the authorization plugin
    (https://discourse.orthanc-server.org/t/user-based-access-control-with-label-based-resource-access/5454).

Fixes:
  - Fixed the token-type for medDream in the Quick Viewer button.
  - Removed the DatePicker icon to provide more room for the text.


1.7.1 (2025-01-22)
==================

Fixes:
  - Added status text in job progress bar.
  - When retrieving a study from a remote DICOM modality, use the default retrieve method:
    C-GET or C-MOVE.
  - Fixed empty PatientName column when querying a remote DICOM modality.
  - Fixed a few small issues when navigating between local and remote studies.


1.7.0 (2024-12-19)
==================

Changes:
  - When Orthanc DB supports "ExtendedFind" (SQLite in 1.12.5+ and PosgreSQL 7.0+):
    - new features in the local studies list:
      - Allow sorting by columns
      - Optimized loading of "most-recent" studies
      - Load the following studies when scrolling to the bottom of the current list.
    - New configuration "EnableLabelsCount" to enable/disable the display of the number of studies with each label.
    - The "MaxStudiesDisplayed" configuration is not taken into account anymore for
      the local study list since we have implemented "infinite-scroll".  However, the option
      is still used when performing remote DicomWEB queries.
    - New configuration "PageLoadSize" that defines the number of items that are loaded when scrolling the study or instance list.
    - The "StudyListContentIfNoSearch" configuration is not taken into account and always considered as "most-recents".
    - New "order-by" argument in the url to open the UI directly on a search result, e.g:
      http://localhost:8042/ui/app/#/filtered-studies?StudyDate=20231213-20241213&order-by=DicomTag,StudyDate,ASC;DicomTag,PatientName,ASC;Metadata,LastUpdate,DESC 
  - Disable some UI components on ReadOnly systems.
  - The study list header is now sticking on top of the screen.
  - Quick report icon: now display the SeriesDate - SeriesDescription in a tooltip.

Fixes:
  - When modifying studies, dates selected from the DatePicker were not always taken into account.
  - Fixed the criteria to display the OHIF Segmentation viewer.
  - Fixed display of invalid dates like 00000000.
  - Fixed compatibility with OHIF 1.4 if OHIF.DataSource is not defined.


1.6.4 (2024-10-10)
==================

Changes:
  - new configuration "EnableViewerQuickButton" to enable/disable a button
    to open a viewer directly from the study list (default value: true).
  - new configuration "EnableReportQuickButton" to enable/disable a button
    to open a PDF report directly from the study list if a PDF report isavailable 
    in the study.  (default value: false).  
    Note that, with Orthanc version up to 1.12.4, this option may slow down the 
    display of the study list but this will be solved in the next Orthanc version.

Fixes:
  - When modifying studies, dates selected from the DatePicker were not always taken into account.
  - Primary viewer icon was not visible when using an external OHIF viewer.
  

1.6.2 (2024-09-23)
==================

Changes:
  - Added a clickable icon to open the primary viewer without expanding the study.
  - Added a clickable icon to open a PDF report without expanding the study.
  - new configuration "CustomFavIconPath" to customize the FavIcon
  - new configuration "CustomTitle" to customize the tab/window title
  - new configurations to modify the PatientName display:
    - "PatientNameCapture", a Javascript regular expression to capture the words of PatientName
    - "PatientNameFormatting", a replacement expression using the captured words

Fixes:
  - Fixed (again) the "UiOptions.EnableApiViewMenu" configuration that was not taken into account.  The API View button was never visible.


1.6.1 (2024-08-29)
==================

Changes:
  - new configuration values for "UiOptions.StudyListColumns":
    - "instancesCount" to show the number of instances in a study
    - "seriesAndInstancesCount" to show the number of series/instances in a study.  This now replaces "seriesCount" in the default configuration.

Fixes:
  - Fixed the "UiOptions.EnableApiViewMenu" configuration that was not taken into account.  The API View button was always visible.
  - [Issue #65](https://github.com/orthanc-server/orthanc-explorer-2/issues/65) SeriesCount column is empty.
  - [Issue #63](https://github.com/orthanc-server/orthanc-explorer-2/issues/63) This/last week selection in Date picker now starts on monday.
  - [Bug #232](https://orthanc.uclouvain.be/bugs/show_bug.cgi?id=232) Clarified compilation instructions and use LOCAL dist folder by default.


1.6.0 (2024-08-02)
==================

Changes:
  - POSSIBLE BREAKING CHANGE: the Osimis viewer button is not listed anymore by default.  In order
    to re-enable it, you must provide it to the "UiOptions.ViewersOrdering" configuration.
  - Orthanc Explorer 2 is now the default Orthanc UI when the plugin is installed.
  - Refactored the Remote Study List when browsing remote DICOM Modalities.  It is now identical to the main
    local study list with a reduced list of actions (only the retrieve action is available).
    - the /ui/app/#/filtered-remote-studies has been replaced by /ui/app/#/filtered-studies?source-type=dicom&remote-source=...
  - Now providing the ability to browse remote DICOMWeb servers.
  - Authorization tokens can be provided in the URL as query args e.g: http://localhost:8042/ui/app/?token=my-token
    or /ui/app/filtered-studies?StudyDescription=PET&token=my-token  and these tokens will be included as HTTP headers 
    in all requests issued by OE2.  Note that the query args must be positioned before the '#' in the URL.
    List of valid tokens are "token", "auth-token", "authorization".
  - Now sorting the results of a search by StudyDate.


Fixes:
  - Labels list was not displayed in the "Permissions" edition UI.
  - Opening /ui/app now redirects to /ui/app/ instead of failing opening the UI.


1.5.1 (2024-07-03)
==================

Fixes:
  - Broken interface when the auth-service was not configured to implement the role/permission API


1.5.0 (2024-06-27)
==================

Changes:
  - Implemented a new permission UI in case you are using OE2 together with Keycloak and an authorisation service that implements a role/permission API.
  - New configuration "AvailableLabels" to forbid creation of labels that are not defined in this list.  If the list is empty, anyone can create any new label.

Fixes:
  - [Issue #57](https://github.com/orthanc-server/orthanc-explorer-2/issues/57) Avoid double calls to tools/find when selecting a label or clicking on `search` button
  - In the settings page, the status of the python plugin was not correct.


1.4.1 (2024-06-05)
==================

Changes:
- Settings: if the [Delayed Deletion plugin](https://orthanc.uclouvain.be/book/plugins/delayed-deletion-plugin.html) 
  is enabled, display the number of files pending deletion.

Fixes:
  - Prevent using invalid characters when adding labels to a study.
  - [Issue #58](https://github.com/orthanc-server/orthanc-explorer-2/issues/58) Upload function only uploads the first file.


1.4.0 (2024-05-16)
==================

Changes:
- Added a "Add series" button to the study to create new series from PDF, images or STL files
  - New configurations:
    - "EnableAddSeries"
    - "AddSeriesDefaultTags"

Fixes:
  - [Issue #51](https://github.com/orthanc-server/orthanc-explorer-2/issues/51) Stop calling /changes once all studies are displayed.
  - [Issue #52](https://github.com/orthanc-server/orthanc-explorer-2/issues/52) Removed duplicate calls to /studies/../series and /series/../instances.
  - [Issue #53](https://github.com/orthanc-server/orthanc-explorer-2/issues/53) Changing selection in Modalities in Study works immediately even if `"StudyListSearchMode": "search-button"`.
  - [Issue #54](https://github.com/orthanc-server/orthanc-explorer-2/issues/54) OHIF disappeared after #52 fix
  - [Issue #55](https://github.com/orthanc-server/orthanc-explorer-2/issues/55) DateFormat not respected + introduced DatePicker in the Modification Modal.
  - Removed duplicate calls to /tools/find in Modification modal


1.3.0 (2024-03-25)
==================

Changes:
- Theming the interface:
  - New configurations:
    - "Theme" to select the default "light" or "dark" theme.
    - "CustomCssPath" to complement the default CSS by a custom one.
    - "CustomLogoPath" to provide your own custom logo from disk.
    - "CustomLogoUrl" to provide your own custom logo from an external url.
    - "DateFormat" to customize the date format in study list and in the date pickers.
- OHIF Integration:
  - Added support for `Segmentation` and `Microscopy` modes.  The `Microscopy`
    mode is disabled by default since it is not stable yet in OHIF.
  - You can now enable/disable OHIF viewer modes by including/removing them
    from the `ViewersOrdering` configuration.
  - OHIF buttons are now visible/hidden depending on the content of the study.
- Configurations:
  - Updated default values for `ViewersIcons` and `ViewersOrdering`.
- Added date pickers in the Remote Study List (when performing searches on
  remote modalities)
- When editing a study, it is now possible to add an `OtherPatientIDs` DICOM tag.

Internals:
  - Updated all JS libraries.


1.2.2 (2024-02-16)
==================

Changes:
- Now keeping labels when modifying a study or a series.  
  This fix requires Orthanc 1.12.3.
- Added Slovenian translations
- Now showing the DICOM header content at instance level (TransferSyntax, ...)
- Instance preview now handles PDF correctly.
- Now displaying #Patients and MaximumPatientCount in Settings.


Fixes:
- fixed delete on multiple selection in Firefox


1.2.1 (2024-01-03)
==================

Fixes:
- Handle quotes correctly when "PatientBirthDate" is defined in "ShowSamePatientStudiesFilter"
- fix #42: Studies disappear from list when unselecting them.
- fix #34: Enable copy button when context is not secure.


1.2.0 (2023-12-19)
==================

Changes:
- Added a button to download multiple studies at once (only available if running an
  Orthanc version > 1.12.1 - not officially released at the time this plugin was
  released)
- Bew configuration "UiOptions.ShowSamePatientStudiesFilter" to list the tags that 
  are used to identify studies belonging to the same patient.
- Added Russian translations
- Added a button to share multiple studies at once.
- Now displaying the MaximumStorageSize in the settings page.
- New configurations "UiOptions.Modifications.SeriesAllowedModes" & "UiOptions.Modifications.SeriesDefaultMode"
  to configure how series are modified.  If only one option is available, it is not displayed and the
  default option is selected.
- In "Modify Patient Tags", allow applying changes even if nothing has changed e.g. to apply all current
  Patient tags to all studies with the same Patient ID.

Fixes:
- fix #43: Missing 'ModalitiesInStudy' in response in remote system QR breaks search result preview.


1.1.3 (2023-10-04)
==================

Fixes:
- fix target of 'open Orthanc Explorer 2' button in legacy OE
- repair tables background color (bug in 1.1.2)

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
