import store from "../store"
import api from "../orthancApi"

export default {
    getResourceTitle(resourceType, patientMainDicomTags, studyMainDicomTags, seriesMainDicomTags, instanceTags) {
        let title = [];

        function addIfExists(title, key, dico) {
            if (key in dico) {
                title.push(dico[key]);
                return true;
            }
            return false;
        };

        if (resourceType == "study" || resourceType == "series") {
            addIfExists(title, "PatientID", patientMainDicomTags)
            addIfExists(title, "StudyID", studyMainDicomTags)
            addIfExists(title, "StudyDate", studyMainDicomTags)
            addIfExists(title, "StudyDescription", studyMainDicomTags)
        }
        if (resourceType == "series" || resourceType == "instance") {
            addIfExists(title, "SeriesNumber", seriesMainDicomTags);
            addIfExists(title, "SeriesDescription", seriesMainDicomTags);
        }
        if (resourceType == "instance") {
            if ("0020,0013" in instanceTags) {
                title.push("instance # " + instanceTags["0020,0013"]["Value"]);
            } else {
                title.push("instance");
            }
        }

        return title.join(" | ");
    },

    replaceResourceTagsInString(template, patientMainDicomTags, studyMainDicomTags, seriesMainDicomTags, instanceTags, resourceId) {
        let output = template;
        let transformedInstanceTags = {};
        if (instanceTags != null) {
            for (const [k, v] of Object.entries(instanceTags)) {
                transformedInstanceTags[v['Name']] = v['Value'];
            }
        }
        
        for (let levelTags of [patientMainDicomTags, studyMainDicomTags, seriesMainDicomTags, transformedInstanceTags]) {
            if (levelTags != null) {
                for (const [k, v] of Object.entries(levelTags)) {
                    output = output.replace("{" + k + "}", v);
                }
            }
        }

        output = output.replace("{UUID}", resourceId);
        output = output.replace(/{[^}]+}/g, 'undefined');
        return output;
    },

    replaceResourceTagsInJson(template, patientMainDicomTags, studyMainDicomTags, seriesMainDicomTags, instanceTags, resourceId) {
        if (template == null) {
            return null;
        }
        let output = {};
        
        for (const [k, v] of Object.entries(template)) {
            if (typeof v === 'string') {
                if (v.indexOf('{') != -1) {
                    output[k] = this.replaceResourceTagsInString(v, patientMainDicomTags, studyMainDicomTags, seriesMainDicomTags, instanceTags, resourceId);
                } else {
                    output[k] = v;
                }
            } else if (Array.isArray(v)) {
                output[k] = [];
                for (const vv of v) {
                    if (typeof vv === 'string') {
                        if (vv.indexOf('{') != -1) {
                            output[k].push(this.replaceResourceTagsInString(v, patientMainDicomTags, studyMainDicomTags, seriesMainDicomTags, instanceTags, resourceId));
                        } else {
                            output[k].push(v);
                        }
                    }
                }
            } else if (typeof v === 'object') {
                output[k] = this.replaceResourceTagsInJson(v, patientMainDicomTags, studyMainDicomTags, seriesMainDicomTags, instanceTags, resourceId);
            } else {
                output[k] = v;
            }
        }
        return output;
    },

    patientNameCapture : "([^\\^]+)\\^?([^\\^]+)?\\^?([^\\^]+)?\\^?([^\\^]+)?\\^?([^\\^]+)?",
    patientNameFormatting : null,
    formatPatientName(originalPatientName) {
        if (originalPatientName && this.patientNameFormatting && this.patientNameCapture) {
            return originalPatientName.replace(new RegExp(this.patientNameCapture), this.patientNameFormatting);
        } else {
            return originalPatientName;
        }
    },

    getPrimaryViewerUrl(level, orthancId, dicomId) {
        if (store.state.configuration.uiOptions.ViewersOrdering.length > 0) {
            for (let viewer of store.state.configuration.uiOptions.ViewersOrdering) {
                if ((["osimis-web-viewer", "stone-webviewer", "volview", "wsi"].indexOf(viewer) != -1 && viewer in store.state.configuration.installedPlugins) ||
                    (viewer.startsWith("ohif") && viewer in store.state.configuration.installedPlugins) ||
                    (viewer.startsWith("ohif") && store.state.configuration.uiOptions.EnableOpenInOhifViewer3) ||
                    (viewer == "meddream" && store.state.configuration.uiOptions.EnableOpenInMedDreamViewer))
                {
                    return this.getViewerUrl(level, orthancId, dicomId, viewer);
                }
            }
        }
        return null;
    },

    getPrimaryViewerTokenType() {
        if (store.state.configuration.uiOptions.ViewersOrdering.length > 0) {
            for (let viewer of store.state.configuration.uiOptions.ViewersOrdering) {
                if (viewer == "meddream") {
                    return "meddream-instant-link";
                } else {
                    return "viewer-instant-link";
                }
            }
        }
        return null;
    },

    getViewerUrl(level, orthancId, dicomId, viewer) {
        if (viewer == 'osimis-web-viewer') {
            return api.getOsimisViewerUrl(level, orthancId);
        } else if (viewer == 'stone-webviewer') {
            return api.getStoneViewerUrl(level, dicomId)
        } else if (viewer == 'volview') {
            return api.getVolViewUrl(level, orthancId);
        } else if (viewer == 'ohif') {
            if (store.state.configuration.ohifDataSource == 'dicom-web') {
                return api.getOhifViewerUrlForDicomWeb('basic', dicomId);
            } else {
                return api.getOhifViewerUrlForDicomJson('basic', dicomId);
            }
        } else if (viewer == 'ohif-vr') {
            return api.getOhifViewerUrl('vr');
        } else if (viewer == 'ohif-tmtv') {
            return api.getOhifViewerUrl('tmtv');
        } else if (viewer == 'ohif-seg') {
            return api.getOhifViewerUrl('seg');
        } else if (viewer == 'ohif-micro') {
            return api.getOhifViewerUrl('microscopy');
        } else if (viewer == 'wsi') {
            return api.getWsiViewerUrl(orthancId); // note: this must be a series ID !
        } else if (viewer == 'meddream') {
            return store.state.configuration.uiOptions.MedDreamViewerPublicRoot + "?study=" + dicomId;
        }
    }
}
