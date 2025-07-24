import axios from "axios"
import store from "./store"
import mime from "mime-types";
import { showSaveFilePicker } from "native-file-system-adapter";


import { orthancApiUrl, oe2ApiUrl } from "./globalConfigurations";

export default {
    updateAuthHeader(headerKey = null) {
        axios.defaults.headers.common[headerKey ?? "token"] = localStorage.getItem(headerKey ?? "vue-token")
    },
    async loadOe2Configuration() {
        return (await axios.get(oe2ApiUrl + "configuration")).data;
    },
    async loadDicomWebServers() {
        return (await axios.get(orthancApiUrl + "dicom-web/servers")).data;
    },
    async loadOrthancPeers() {
        return (await axios.get(orthancApiUrl + "peers")).data;
    },
    async loadDicomModalities() {
        return (await axios.get(orthancApiUrl + "modalities?expand")).data;
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
    async deleteResources(resourcesIds) {
        return axios.post(orthancApiUrl + "tools/bulk-delete", {
            "Resources": resourcesIds
        });
    },
    async cancelFindStudies() {
        if (window.axiosFindStudiesAbortController) {
            window.axiosFindStudiesAbortController.abort();
            window.axiosFindStudiesAbortController = null;
        }
    },
    async findStudies(filterQuery, labels, LabelsConstraint, orderBy, since) {
        await this.cancelFindStudies();
        window.axiosFindStudiesAbortController = new AbortController();

        let limit = store.state.configuration.uiOptions.MaxStudiesDisplayed;
        if (store.state.configuration.hasExtendedFind) {
            limit = store.state.configuration.uiOptions.PageLoadSize;
        }

        let payload = {
            "Level": "Study",
            "Limit": limit,
            "Query": filterQuery,
            "RequestedTags": store.state.configuration.requestedTagsForStudyList,
            "Expand": true
        };
        
        if (labels && labels.length > 0) {
            payload["Labels"] = labels;
            payload["LabelsConstraint"] = LabelsConstraint;
        }

        if (orderBy && orderBy.length > 0) {
            payload["OrderBy"] = orderBy;
        }

        if (since) {
            payload["Since"] = since;
        }

        let response = {
            "studies": (await axios.post(orthancApiUrl + "tools/find", payload, 
                {
                    signal: window.axiosFindStudiesAbortController.signal
                })).data
        };
        response["is-complete"] = response["studies"].length < limit; // we have received all available studies
        return response;
    },
    async getMostRecentStudiesExtended(label) {
        await this.cancelFindStudies();
        window.axiosFindStudiesAbortController = new AbortController();

        let payload = {
            "Level": "Study",
            "Limit": store.state.configuration.uiOptions.PageLoadSize,
            "Query": {},
            "RequestedTags": store.state.configuration.requestedTagsForStudyList,
            "OrderBy" : [{
                'Type': 'Metadata',
                'Key': 'LastUpdate',
                'Direction': 'DESC'
            }],
            "Expand": true
        };
        if (label) {
            payload["Labels"] = [label];
            payload["LabelsConstraint"] = "All";
        }

        let response = {
            "studies": (await axios.post(orthancApiUrl + "tools/find", payload, 
                {
                    signal: window.axiosFindStudiesAbortController.signal
                })).data
        };
        response["is-complete"] = response["studies"].length < limit; // we have received all available studies
        return response;
    },
    async getLastChangeId() {
        const response = (await axios.get(orthancApiUrl + "changes?last"));
        return response.data["Last"];
    },
    async getChanges(since, limit) {
        const response = (await axios.get(orthancApiUrl + "changes?since=" + since + "&limit=" + limit));
        return response.data;
    },
    async getChangesExtended(to, limit, filter = []) {
        let url = orthancApiUrl + "changes?to=" + to + "&limit=" + limit;
        if (filter.length > 0) {
            url += "&type=" + filter.join(";")
        }
        const response = (await axios.get(url));
        return response.data;
    },
    async getSamePatientStudies(patientTags, tags) {
        if (!tags || tags.length == 0) {
            console.error("Unable to getSamePatientStudies if 'tags' is not defined or empty");
            return {};
        }
        
        let query = {};
        for (let tag of tags) {
            if (tag in patientTags) {
                query[tag] = patientTags[tag];
            }
        }
        const response = (await axios.post(orthancApiUrl + "tools/find", {
            "Level": "Study",
            "Query": query,
            "Expand": false
        }));
        return response.data;
    },
    async findPatient(patientId) {
        const response = (await axios.post(orthancApiUrl + "tools/lookup", patientId));
        if (response.data.length == 1) {
            const patient = (await axios.get(orthancApiUrl + "patients/" + response.data[0]['ID']));
            return patient.data;
        } else {
            return null;
        }
    },
    async findStudy(studyInstanceUid) {
        const response = (await axios.post(orthancApiUrl + "tools/lookup", studyInstanceUid));
        if (response.data.length == 1) {
            const study = (await axios.get(orthancApiUrl + "studies/" + response.data[0]['ID']));
            return study.data;
        } else {
            return null;
        }
    },
    async studyExists(studyInstanceUid) {
        const response = (await axios.post(orthancApiUrl + "tools/lookup", studyInstanceUid));
        return response.data.length >= 1;
    },
    async mergeSeriesInExistingStudy({seriesIds, targetStudyId, keepSource}) {
        const response = (await axios.post(orthancApiUrl + "studies/" + targetStudyId + "/merge", {
            "Resources": seriesIds,
            "KeepSource": keepSource,
            "Synchronous": false
        }));
        return response.data['ID'];
    },
    async cancelRemoteDicomFind() {
        if (window.axiosRemoteDicomFindAbortController) {
            window.axiosRemoteDicomFindAbortController.abort();
            window.axiosRemoteDicomFindAbortController = null;
        }
    },
    async remoteDicomFind(level, remoteModality, filterQuery, isUnique) {
        if (isUnique) {
            await this.cancelRemoteDicomFind();
            window.axiosRemoteDicomFindAbortController = new AbortController();
        }
        
        try {
            let axiosOptions = {}
            if (isUnique) {
                axiosOptions['signal'] = window.axiosRemoteDicomFindAbortController.signal
            }
            const queryResponse = (await axios.post(orthancApiUrl + "modalities/" + remoteModality + "/query", {
                    "Level": level,
                    "Query": filterQuery
                }, 
                axiosOptions
                )).data;
            // console.log(queryResponse);
            const answers = (await axios.get(orthancApiUrl + "queries/" + queryResponse["ID"] + "/answers?expand&simplify")).data;
            // console.log(answers);
            return answers;
        } catch (err)
        {
            console.log("Error during query:", err);  // TODO: display error to user
            return {};
        }
    },
    async qidoRs(level, remoteServer, filterQuery, isUnique){
        if (isUnique) {
            await this.cancelQidoRs();
            window.axiosQidoRsAbortController = new AbortController();
        }
        
        try {
            let axiosOptions = {}
            if (isUnique) {
                axiosOptions['signal'] = window.axiosQidoRsAbortController.signal
            }
            let uri = null;
            if (level == "Study") {
                uri = "/studies";
            } else if (level == "Series") {
                uri = "/studies/" + filterQuery["StudyInstanceUID"] + "/series";
                delete filterQuery["StudyInstanceUID"];  // we don't need it the filter since it is in the url
            } else if (level == "Instance") {
                uri = "/studies/" + filterQuery["StudyInstanceUID"] + "/series/" + filterQuery["SeriesInstanceUID"] + "/instances";
                delete filterQuery["StudyInstanceUID"];  // we don't need it the filter since it is in the url
                delete filterQuery["SeriesInstanceUID"];
            }
            let args = {...filterQuery};
            args["limit"] = String(store.state.configuration.uiOptions.MaxStudiesDisplayed);
            args["fuzzymatching"] = "true";
            
            const queryResponse = (await axios.post(orthancApiUrl + "dicom-web/servers/" + remoteServer + "/qido", {
                    "Uri": uri,
                    "Arguments": args
                }, 
                axiosOptions
                )).data;
            // transform the response into something similar to a DICOM C-Find API response
            let responses = [];
            for (let qr of queryResponse) {
                let r = {}
                for (const [k, v] of Object.entries(qr)) {
                    if (v.Value) {
                        r[v.Name] = v.Value;
                    }
                }
                responses.push(r);
            }
            return responses;
        } catch (err)
        {
            console.log("Error during query:", err);  // TODO: display error to user
            return {};
        }
    },
    async cancelQidoRs() {
        if (window.axiosQidoRsAbortController) {
            window.axiosQidoRsAbortController.abort();
            window.axiosQidoRsAbortController = null;
        }
    },
    async wadoRsRetrieve(remoteServer, resources){
        const retrieveJob = (await axios.post(orthancApiUrl + "dicom-web/servers/" + remoteServer + "/retrieve", {
                "Resources": resources,
                "Asynchronous": true
            }
            )).data;
        return retrieveJob["ID"];
    },
    async remoteDicomRetrieveResource(level, remoteModality, filterQuery) {
        return this.remoteDicomRetrieveResources(level, remoteModality, [filterQuery]);
    },
    async remoteDicomRetrieveResources(level, remoteModality, filterQueries) {
        const retrieveMethod = this.getRetrieveMethod(remoteModality);
        const targetAet = this.getRetrieveAet(remoteModality);

        let uriSegment = "/get";
        let query = {
            "Level": level,
            "Resources" : filterQueries,
            "Synchronous": false
        }

        if (retrieveMethod == "C-MOVE") {
            query["TargetAet"] = targetAet;
            uriSegment = "/move";
        }

        const response = (await axios.post(orthancApiUrl + "modalities/" + remoteModality + uriSegment, query));
        return response.data['ID'];
    },
    async remoteModalityEcho(remoteModality) {
        return axios.post(orthancApiUrl + "modalities/" + remoteModality + "/echo", {});
    },
    async uploadFile(filecontent) {
        return (await axios.post(orthancApiUrl + "instances", filecontent)).data;
    },
    async createDicom(parentId, content, tags) {
        return (await axios.post(orthancApiUrl + "tools/create-dicom", {
            "Parent": parentId,
            "Tags": tags,
            "Content": content
        })).data
    },
    async getPatient(orthancId) {
        return (await axios.get(orthancApiUrl + "patients/" + orthancId)).data;
    },
    async getStudy(orthancId) {
        // returns the same result as a findStudies (including RequestedTags !)
        return (await axios.get(orthancApiUrl + "studies/" + orthancId + "?requestedTags=" + store.state.configuration.requestedTagsForStudyList.join(';'))).data;
    },
    async getStudyLabels(orthancId) {
        // returns the same result as a findStudies (including RequestedTags !)
        return (await axios.get(orthancApiUrl + "studies/" + orthancId + "/labels")).data;
    },
    async getStudySeries(orthancId) {
        return (await axios.get(orthancApiUrl + "studies/" + orthancId + "/series")).data;
    },
    async getSeriesInstances(orthancId) {
        return (await axios.get(orthancApiUrl + "series/" + orthancId + "/instances")).data;
    },
    async getSeriesInstancesExtended(orthancId, since) {
        const limit = store.state.configuration.uiOptions.PageLoadSize;
        let payload = {
            "Level": "Instance",
            "Limit": limit,
            "ParentSeries": orthancId,
            "Query": {},
            "OrderBy" : [
                { "Type": "MetadataAsInt",
                  "Key": "IndexInSeries",
                  "Direction": "ASC"  
                }
            ],
            "Expand": true
        };

        if (since) {
            payload["Since"] = since;
        }
        
        const response = (await axios.post(orthancApiUrl + "tools/find", payload));
        return response.data;
    },
    async getStudyInstances(orthancId) {
        return (await axios.get(orthancApiUrl + "studies/" + orthancId + "/instances")).data;
    },
    async getStudyInstancesExpanded(orthancId, requestedTags) {
        let url = orthancApiUrl + "studies/" + orthancId + "/instances?expanded";
        if (requestedTags && requestedTags.length > 0) {
            url += "&requestedTags=" + requestedTags.join(";")
        }
        return (await axios.get(url)).data;
    },
    async getSeriesParentStudy(orthancId) {
        return (await axios.get(orthancApiUrl + "series/" + orthancId + "/study")).data;
    },
    async getInstanceParentStudy(orthancId) {
        return (await axios.get(orthancApiUrl + "instances/" + orthancId + "/study")).data;
    },
    async getResourceStudy(orthancId, level) {
        if (level == "study") {
            return (await this.getStudy(orthancId));
        } else if (level == "series") {
            return (await this.getSeriesParentStudy(orthancId));
        } else if (level == "instance") {
            return (await this.getInstanceParentStudy(orthancId));
        } else {
            console.error("unsupported level for getResourceStudyId", level);
        }
    },
    async getInstanceTags(orthancId) {
        return (await axios.get(orthancApiUrl + "instances/" + orthancId + "/tags")).data;
    },
    async getSimplifiedInstanceTags(orthancId) {
        return (await axios.get(orthancApiUrl + "instances/" + orthancId + "/tags?simplify")).data;
    },
    async getInstanceHeader(orthancId) {
        return (await axios.get(orthancApiUrl + "instances/" + orthancId + "/header")).data;
    },
    async getStatistics() {
        return (await axios.get(orthancApiUrl + "statistics")).data;
    },
    async getPermissionsList() {
        return (await axios.get(orthancApiUrl + "auth/settings/permissions")).data;
    },
    async getRolesConfig() {
        return (await axios.get(orthancApiUrl + "auth/settings/roles")).data;
    },
    async setRolesConfig(roles) {
        return (await axios.put(orthancApiUrl + "auth/settings/roles", roles)).data;
    },
    async getDelayedDeletionStatus() {
        return (await axios.get(orthancApiUrl + "plugins/delayed-deletion/status")).data;
    },
    async getAdvancedStorageStatus() {
        return (await axios.get(orthancApiUrl + "plugins/advanced-storage/status")).data;
    },
    async getHousekeeperStatus() {
        return (await axios.get(orthancApiUrl + "plugins/housekeeper/status")).data;
    },
    async generateUid(level) {
        return (await axios.get(orthancApiUrl + "tools/generate-uid?level=" + level)).data;
    },
    async setVerboseLevel(level) {
        await axios.put(orthancApiUrl + "tools/log-level", level);
    },

    async getVerboseLevel() {
        return (await axios.get(orthancApiUrl + "tools/log-level")).data;
    },

    async anonymizeResource({resourceLevel, orthancId, replaceTags={}, removeTags=[]}) {
        const response = (await axios.post(orthancApiUrl + this.pluralizeResourceLevel(resourceLevel) + "/" + orthancId + "/anonymize", {
            "Replace": replaceTags,
            "Remove": removeTags,
            "KeepSource": true,
            "Force": true,
            "Synchronous": false
        }))

        return response.data['ID'];
    },

    async modifyResource({resourceLevel, orthancId, replaceTags={}, removeTags=[], keepTags=[], keepSource}) {
        const response = (await axios.post(orthancApiUrl + this.pluralizeResourceLevel(resourceLevel) + "/" + orthancId + "/modify", {
            "Replace": replaceTags,
            "Remove": removeTags,
            "Keep": keepTags,
            "KeepSource": keepSource,
            "KeepLabels": true,
            "Force": true,
            "Synchronous": false
        }))

        return response.data['ID'];
    },

    async loadAllLabels() {
        const response = (await axios.get(orthancApiUrl + "tools/labels"));
        return response.data;
    },

    async addLabel({studyId, label}) {
        await axios.put(orthancApiUrl + "studies/" + studyId + "/labels/" + label, "");
        return label;
    },

    async removeLabel({studyId, label}) {
        await axios.delete(orthancApiUrl + "studies/" + studyId + "/labels/" + label);
        return label;
    },

    async removeAllLabels(studyId) {
        const labels = await this.getLabels(studyId);
        let promises = [];
        for (let label of labels) {
            promises.push(this.removeLabel({
                studyId: studyId,
                label: label
            }));
        }
        await Promise.all(promises);
        return labels;
    },

    async getLabels(studyId) {
        const response = (await axios.get(orthancApiUrl + "studies/" + studyId + "/labels"));
        return response.data;
    },

    async updateLabels({studyId, labels}) {
        const currentLabels = await this.getLabels(studyId);
        const labelsToRemove = currentLabels.filter(x => !labels.includes(x));
        const labelsToAdd = labels.filter(x => !currentLabels.includes(x));
        let promises = [];

        // console.log("labelsToRemove: ", labelsToRemove);
        // console.log("labelsToAdd: ", labelsToAdd);
        for (const label of labelsToRemove) {
            promises.push(this.removeLabel({
                studyId: studyId,
                label: label
            }));
        }
        for (const label of labelsToAdd) {
            promises.push(this.addLabel({
                studyId: studyId,
                label: label
            }));
        }
        await Promise.all(promises);
        return labelsToAdd.length > 0 || labelsToRemove.length > 0;
    },



    async createToken({tokenType, resourcesIds, level, validityDuration=null, id=null, expirationDate=null}) {
        let body = {
            "Resources" : [],
            "Type": tokenType
        }

        for (let resourceId of resourcesIds) {
            // the authorization are performed at study level -> get parent study id if needed
            const study = await this.getResourceStudy(resourceId, level);
            body["Resources"].push({
                "OrthancId": study["ID"],
                "DicomUid": study["MainDicomTags"]["StudyInstanceUID"],
                "Level": 'study'
            })
        }

        if (validityDuration != null) {
           body["ValidityDuration"] = validityDuration;
        }

        if (expirationDate != null) {
            body["ExpirationDate"] = expirationDate.toJSON();
        }

        if (id != null) {
            body["Id"] = id;
        }

        const response = (await axios.put(orthancApiUrl + "auth/tokens/" + tokenType, body));
        // console.log(response);
        
        return response.data;
    },
    async parseToken(tokenKey, tokenValue) {
        const response = (await axios.post(orthancApiUrl + "auth/tokens/decode", {
            "TokenKey": tokenKey,
            "TokenValue": tokenValue
        }))

        return response.data;
    },
    async getLabelStudyCount(label) {
        const response = (await axios.post(orthancApiUrl + "tools/count-resources", {
            "Level": "Study",
            "Query": {},
            "Labels": [label],
            "LabelsConstraint" : "All"
        }));
        return response.data["Count"];
    },
    async downloadFileWithAuthHeaders(url) {
        fetch(url, {  // we must use fetch because axios does not provide a stream response
            method: "GET",
            headers: axios.defaults.headers.common
        })
        .then(async (response) => {
            let responseStream = response.body;
            // console.info(response);

            let suggestedFileName = null;
            if (response.headers.has('content-disposition')) {
                suggestedFileName = response.headers.get('content-disposition').split('"')[1]
            }
                
            const extension = mime.extension(response.headers.get('content-type'));
            let acceptedTypes = [];
            acceptedTypes.push({ accept: { [response.headers.get('content-type').split(",")[0]]: ["." + extension] } });

            const fileHandle = await showSaveFilePicker({
                _preferPolyfill: true,
                suggestedName: suggestedFileName,
                types: acceptedTypes,
                excludeAcceptAllOption: false,
            }).catch((error) => console.error(error));;

            let writableStream = await (fileHandle).createWritable();
            await responseStream.pipeTo(writableStream);
            console.log(suggestedFileName + " downloaded");
        })
        .catch((error) => {

        })
    },
    async commitInbox(commitUrl, orthancStudiesIds, formFields) {
        const response = (await axios.post(orthancApiUrl + commitUrl, {
            "OrthancStudiesIds": orthancStudiesIds,
            "FormFields": formFields
        }))

        return response.data;
    },
    async monitorInboxProcessing(monitorUrl, commitResponse) {
        const response = (await axios.post(orthancApiUrl + monitorUrl, commitResponse))
        return response.data;
    },
    async validateInboxForm(validationUrl, formFields) {
        const response = (await axios.post(orthancApiUrl + validationUrl, {
            "FormFields": formFields
        }))

        return response.data;
    },
    async getAuditLogsForResource(filters) {
        const getArguments = new URLSearchParams();
        if (filters) {
            for (const [key, value] of Object.entries(filters)) {
                getArguments.append(key, value);
            }
        }

        return (await axios.get(orthancApiUrl + "auth/audit-logs?" + getArguments.toString())).data;
    },

    ////////////////////////////////////////// HELPERS
    getOsimisViewerUrl(level, resourceOrthancId) {
        return orthancApiUrl + 'osimis-viewer/app/index.html?' + level + '=' + resourceOrthancId;
    },
    getStoneViewerUrl(level, resourceDicomUid) {
        return orthancApiUrl + 'stone-webviewer/index.html?' + level + '=' + resourceDicomUid;
    },
    getVolViewUrl(level, resourceOrthancId) {
        const volViewVersion = store.state.configuration.installedPlugins.volview.Version;
        const urls = 'urls=[../' + this.pluralizeResourceLevel(level) + '/' + resourceOrthancId + '/archive]';
        if (volViewVersion == '1.0') {
            return orthancApiUrl + 'volview/index.html?' + urls;
        } else {
            return orthancApiUrl + 'volview/index.html?names=[archive.zip]&' + urls;
        }
    },
    getWsiViewerUrl(seriesOrthancId) {
        return orthancApiUrl + 'wsi/app/viewer.html?series=' + seriesOrthancId;
    },
    getStoneViewerUrlForBulkStudies(studiesDicomIds) {
        return orthancApiUrl + 'stone-webviewer/index.html?study=' + studiesDicomIds.join(",");
    },
    getOhifViewerUrlForDicomJson(mode, resourceOrthancId) {
        if (mode == 'basic') {
            return store.state.configuration.uiOptions.OhifViewer3PublicRoot + 'viewer?url=../studies/' + resourceOrthancId + "/ohif-dicom-json";
        } else if (mode == 'vr') {
            return store.state.configuration.uiOptions.OhifViewer3PublicRoot + 'viewer?hangingprotocolId=mprAnd3DVolumeViewport&url=../studies/' + resourceOrthancId + "/ohif-dicom-json";
        } else if (mode == 'tmtv') {
            return store.state.configuration.uiOptions.OhifViewer3PublicRoot + 'tmtv?url=../studies/' + resourceOrthancId + "/ohif-dicom-json";
        } else if (mode == 'seg') {
            return store.state.configuration.uiOptions.OhifViewer3PublicRoot + 'segmentation?url=../studies/' + resourceOrthancId + "/ohif-dicom-json";
        } else if (mode == 'microscopy') {
            return store.state.configuration.uiOptions.OhifViewer3PublicRoot + 'microscopy?url=../studies/' + resourceOrthancId + "/ohif-dicom-json";
        }
    },
    getOhifViewerUrlForDicomWeb(mode, resourceDicomUid) {
        if (store.state.configuration.uiOptions.EnableOpenInOhifViewer3) {
            if (mode == 'basic') {
                return store.state.configuration.uiOptions.OhifViewer3PublicRoot + 'viewer?StudyInstanceUIDs=' + resourceDicomUid;
            } else if (mode == 'vr') {
                return store.state.configuration.uiOptions.OhifViewer3PublicRoot + 'viewer?hangingprotocolId=mprAnd3DVolumeViewport&StudyInstanceUIDs=' + resourceDicomUid;
            } else if (mode == 'tmtv') {
                return store.state.configuration.uiOptions.OhifViewer3PublicRoot + 'tmtv?StudyInstanceUIDs=' + resourceDicomUid;
            } else if (mode == 'seg') {
                return store.state.configuration.uiOptions.OhifViewer3PublicRoot + 'segmentation?StudyInstanceUIDs=' + resourceDicomUid;
            } else if (mode == 'microscopy') {
                return store.state.configuration.uiOptions.OhifViewer3PublicRoot + 'microscopy?StudyInstanceUIDs=' + resourceDicomUid;
            }
        } else {
            return store.state.configuration.uiOptions.OhifViewerPublicRoot + 'Viewer/' + resourceDicomUid;
        }
    },
    getOhifViewerUrlForDicomWebBulkStudies(mode, studiesDicomIds) {
        if (store.state.configuration.uiOptions.EnableOpenInOhifViewer3) {
            return this.getOhifViewerUrlForDicomWeb(mode, studiesDicomIds.join(","));
        } else {
            return null;
        }
    },
    getInstancePreviewUrl(orthancId) {
        return orthancApiUrl + "instances/" + orthancId + "/preview";
    },
    getInstancePdfUrl(orthancId) {
        return orthancApiUrl + "instances/" + orthancId + "/pdf";
    },
    getInstanceDownloadUrl(orthancId, filename) {
        let filenameArgument = "";
        if (filename) {
            filenameArgument = "?filename=" + encodeURIComponent(filename);
        }
        return orthancApiUrl + "instances/" + orthancId + "/file" + filenameArgument;
    },
    getDownloadZipUrl(level, resourceOrthancId, filename) {
        let filenameArgument = "";
        if (filename) {
            filenameArgument = "?filename=" + encodeURIComponent(filename);
        }
        return orthancApiUrl + this.pluralizeResourceLevel(level) + '/' + resourceOrthancId + '/archive' + filenameArgument;
    },
    getBulkDownloadZipUrl(resourcesOrthancId) {
        if (resourcesOrthancId.length > 0)
        {
            return orthancApiUrl + "tools/create-archive?resources=" + resourcesOrthancId.join(',');
        }
        return undefined;
    },
    getBulkDownloadDicomDirUrl(resourcesOrthancId) {
        if (resourcesOrthancId.length > 0)
        {
            return orthancApiUrl + "tools/create-media?resources=" + resourcesOrthancId.join(',');
        }
        return undefined;
    },
    getDownloadDicomDirUrl(level, resourceOrthancId, filename) {
        let filenameArgument = "";
        if (filename) {
            filenameArgument = "?filename=" + encodeURIComponent(filename);
        }
        return orthancApiUrl + this.pluralizeResourceLevel(level) + '/' + resourceOrthancId + '/media' + filenameArgument;
    },
    getApiUrl(level, resourceOrthancId, subroute) {
        return orthancApiUrl + this.pluralizeResourceLevel(level) + '/' + resourceOrthancId + subroute;
    },
    getRetrieveMethod(modality) {
        const defaultRetrieveMethod = store.state.configuration.system.DefaultRetrieveMethod;
        if (store.state.configuration.queryableDicomModalities[modality].RetrieveMethod == "SystemDefault") {
            return defaultRetrieveMethod;
        } else {
            return store.state.configuration.queryableDicomModalities[modality].RetrieveMethod;
        }
    },
    getRetrieveAet(modality) {
        const defaultRetrieveAet = store.state.configuration.system.DicomAet;
        if (store.state.configuration.queryableDicomModalities[modality].LocalAet == "") {
            return defaultRetrieveAet;
        } else {
            return store.state.configuration.queryableDicomModalities[modality].LocalAet;
        }
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
