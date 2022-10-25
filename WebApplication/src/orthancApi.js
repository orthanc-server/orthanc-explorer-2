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
            "PatientBirthDate",
            "PatientName",
            "PatientID",
            "StudyDescription",
            "StudyDate",
            "modalities",
            "AccessionNumber",
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
        
        return response.data['ID'];
    },
    async sendToOrthancPeer(resourcesIds, destination) {
        const response = (await axios.post(orthancApiUrl + "peers/" + destination + "/store", {
            "Resources" : resourcesIds,
            "Synchronous": false
        }));
        
        return response.data['ID'];
    },
    async sendToOrthancPeerWithTransfers(resources, destination) {
        const response = (await axios.post(orthancApiUrl + "transfers/send", {
            "Resources" : resources,
            "Compression": "gzip",
            "Peer": destination,
            "Synchronous": false
        }));
        
        return response.data['ID'];
    },
    async sendToDicomModality(resourcesIds, destination) {
        const response = (await axios.post(orthancApiUrl + "modalities/" + destination + "/store", {
            "Resources" : resourcesIds,
            "Synchronous": false
        }));
        
        return response.data['ID'];
    },
    async getJobStatus(jobId) {
        const response = (await axios.get(orthancApiUrl + "jobs/" + jobId));
        return response.data;
    },
    async deleteResource(level, orthancId) {
        return axios.delete(orthancApiUrl + this.pluralizeResourceLevel(level) + "/" + orthancId);
    },
    async cancelFindStudies() {
        if (window.axioFindStudiesAbortController) {
            window.axioFindStudiesAbortController.abort();
            window.axioFindStudiesAbortController = null;
        }
    },
    async findStudies(filterQuery) {
        await this.cancelFindStudies();
        window.axioFindStudiesAbortController = new AbortController();

        return axios.post(orthancApiUrl + "tools/find", {
                "Level": "Study",
                "Limit": store.state.configuration.uiOptions.MaxStudiesDisplayed,
                "Query": filterQuery,
                "RequestedTags": [
                    "ModalitiesInStudy"
                ],
                "Expand": true
            }, 
            {
                signal: window.axioFindStudiesAbortController.signal
            });
    },
    async cancelRemoteDicomFindStudies() {
        if (window.axioRemoteDicomFindStudiesAbortController) {
            window.axioRemoteDicomFindStudiesAbortController.abort();
            window.axioRemoteDicomFindStudiesAbortController = null;
        }
    },
    async remoteDicomFindStudies(remoteModality, filterQuery) {
        await this.cancelRemoteDicomFindStudies();
        window.axioRemoteDicomFindStudiesAbortController = new AbortController();

        try {
            const queryResponse = (await axios.post(orthancApiUrl + "modalities/" + remoteModality + "/query", {
                    "Level": "Study",
                    "Query": filterQuery
                }, 
                {
                    signal: window.axioRemoteDicomFindStudiesAbortController.signal
                })).data;
            console.log(queryResponse);
            const answers = (await axios.get(orthancApiUrl + "queries/" + queryResponse["ID"] + "/answers?expand&simplify")).data;
            console.log(answers);
            return answers;
        } catch (err)
        {
            console.log("Error during query:", err);  // TODO: display error to user
            return {};
        }

    },
    async remoteDicomRetrieveStudy(remoteModality, filterQuery, targetAet, level) {
        const response = (await axios.post(orthancApiUrl + "modalities/" + remoteModality + "/move", {
            "Level": level,
            "Resources" : [
                filterQuery
            ],
            "TargetAet": targetAet,
            "Synchronous": false
        }));
        
        return response.data['ID'];
    },
    async remoteModalityEcho(remoteModality) {
        return axios.post(orthancApiUrl + "modalities/" + remoteModality + "/echo", {});
    },
    async uploadFile(filecontent) {
        return axios.post(orthancApiUrl + "instances", filecontent);
    },
    async getStudy(orthancId) {
        // returns the same result as a findStudies (including RequestedTags !)
        return axios.get(orthancApiUrl + "studies/" + orthancId + "?requestedTags=ModalitiesInStudy");
    },
    async getStudySeries(orthancId) {
        return axios.get(orthancApiUrl + "studies/" + orthancId + "/series");
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

    async setVerboseLevel(level) {
        await axios.put(orthancApiUrl + "tools/log-level", level);
    },

    async getVerboseLevel() {
        return (await axios.get(orthancApiUrl + "tools/log-level")).data;
    },

    async shareStudy(orthancId, dicomId, anonymized, expirationInDays) {
        let expirationDate = null;
        
        if (expirationInDays > 0) {
            let today = new Date();
            expirationDate = new Date();
            expirationDate.setDate(today.getDate() + expirationInDays);
        }
                
        const response = (await axios.put(oe2ApiUrl + "shares", {
            "dicom-uid": dicomId,
            "orthanc-id" : orthancId,
            "anonymized": anonymized,
            "expiration-date": expirationDate
        }));
        
        return response.data['url'];
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
    }
}