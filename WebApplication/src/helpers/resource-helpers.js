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


}
