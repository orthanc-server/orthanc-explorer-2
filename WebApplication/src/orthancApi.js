import axios from "axios"
import store from "./store"

import { orthancApiUrl, oe2ApiUrl } from "./globalConfigurations";


// default config only used when running dev against an Orthanc which does not have the latest plugin
const _defaultConfig = {
    "UiOptions" : {
        "EnableUpload": true,
        "EnableDeleteResources": true,
        "EnableApiViewMenu": true,
        "EnableSettings": true,
        "MaxStudiesDisplayed": 50,
        "UploadReportTags" : [
            "AccessionNumber", 
            "StudyDate", 
            "PatientName", 
            "PatientID", 
            "StudyDescription"
        ],
        "UploadReportMaxTags": 2,
        "StudyListColumns" : [
            "StudyDate",
            "AccessionNumber",
            "PatientID",
            "PatientName",
            "PatientBirthDate",
            "StudyDescription",
            "modalities",
            "seriesCount"
        ]
    },
    "Plugins": {
		"dicom-web" : {
			"Description" : "Implementation of DICOMweb (QIDO-RS, STOW-RS and WADO-RS) and WADO-URI.",
			"Enabled" : true,
			"ExtendsOrthancExplorer" : true,
			"ID" : "dicom-web",
			"RootUri" : "../dicom-web/app/client/index.html",
			"Version" : "mainline"
		},
        "osimis-web-viewer": {
            "Description" : "Provides a Web viewer of DICOM series within Orthanc.",
            "ExtendsOrthancExplorer" : true,
            "ID" : "osimis-web-viewer",
            "Version" : "1.4.2.0-9d9eff4",
			"Enabled" : false,
        },
		"stone-webviewer" : 
		{
			"Enabled" : true,
			"ExtendsOrthancExplorer" : true,
			"ID" : "stone-webviewer",
			"Version" : "2.2"
		}
    },
}

export default {
    async loadOe2Configuration() {
        try {
            return (await axios.get(oe2ApiUrl + "configuration")).data;
        } catch (e) {
            console.error("Error while loading OE2 configuration: ", e);
            return _defaultConfig;
        }
    },
    async loadDicomWebServers() {
        return (await axios.get(orthancApiUrl + "dicom-web/servers")).data;
    },
    async loadOrthancPeers() {
        return (await axios.get(orthancApiUrl + "peers")).data;
    },
    async loadDicomModalities() {
        return (await axios.get(orthancApiUrl + "modalities")).data;
    },
    async loadSystem() {
        return (await axios.get(orthancApiUrl + "system")).data;
    },
    async sendToDicomWebServer(resourcesIds, destination) {
        const response = (await axios.post(orthancApiUrl + "dicom-web/servers/" + destination + "/stow", {
            "Resources" : resourcesIds,
            "Synchronous": false
        }));
        
        console.log("TODO: job for dicom-web stow", response);
    },
    async deleteResource(level, orthancId) {
        return axios.delete(orthancApiUrl + this.pluralizeResourceLevel(level) + "/" + orthancId);
    },
    async findStudies(filterQuery) {
        return axios.post(orthancApiUrl + "tools/find", {
                "Level": "Study",
                "Expand": false,
                "Limit": store.state.configuration.uiOptions.MaxStudiesDisplayed,
                "Query": filterQuery,
            });
    },
    async uploadFile(filecontent) {
        return axios.post(orthancApiUrl + "instances", filecontent);
    },
    async getStudy(orthancId) {
        return axios.get(orthancApiUrl + "studies/" + orthancId);
    },
    async getExpandedSeries(orthancId) {
        return axios.get(orthancApiUrl + "series/" + orthancId + "?expand");
    },
    async getSeriesInstances(orthancId) {
        return axios.get(orthancApiUrl + "series/" + orthancId + "/instances");
    },
    async getInstanceTags(orthancId) {
        return axios.get(orthancApiUrl + "instances/" + orthancId + "/tags");
    },
    async getInstanceHeader(orthancId) {
        return axios.get(orthancApiUrl + "instances/" + orthancId + "/header");
    },
    async getStatistics() {
        return axios.get(orthancApiUrl + "statistics");
    },

    ////////////////////////////////////////// HELPERS
    getOsimisViewerUrl(level, resourceOrthancId) {
        return orthancApiUrl + 'osimis-viewer/app/index.html?' + level + '=' + resourceOrthancId;
    },
    getStoneViewerUrl(level, resourceDicomUid) {
        return orthancApiUrl + 'stone-webviewer/index.html?' + level + '=' + resourceDicomUid;
    },    
    getInstancePreviewUrl(orthancId) {
        return orthancApiUrl + "instances/" + orthancId + "/preview";
    },
    getInstanceDownloadUrl(orthancId) {
        return orthancApiUrl + "instances/" + orthancId + "/file";
    },
    getDownloadZipUrl(level, resourceOrthancId) {
        return orthancApiUrl + this.pluralizeResourceLevel(level) + '/' + resourceOrthancId + '/archive';
    },
    getDownloadDicomDirUrl(level, resourceOrthancId) {
        return orthancApiUrl + this.pluralizeResourceLevel(level) + '/' + resourceOrthancId + '/media';
    },
    getApiUrl(level, resourceOrthancId, subroute) {
        return orthancApiUrl + this.pluralizeResourceLevel(level) + '/' + resourceOrthancId + subroute;
    },

    pluralizeResourceLevel(level) {
        if (level == "study") {
            return "studies"
        } else if (level == "instance") {
            return "instances"
        } else if (level == "patient") {
            return "patients"
        } else if (level == "series") {
            return "series"
        }
    },
}