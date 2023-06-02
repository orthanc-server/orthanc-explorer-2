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

    toDicomDate(date) {
        return (date.getFullYear() * 10000 + (date.getMonth()+1) * 100 + date.getDate()).toString();
    },
    parseDateForDatePicker(str) {
        let match = null;
        match = str.match(/^(\d{4})(\d{1,2})(\d{1,2})$/); // yyyymmdd (DICOM)
        if (match) {
            return [new Date(match[1], match[2]-1, match[3]), null];
        }
        match = str.match(/^(\d{4})(\d{1,2})(\d{1,2})\-(\d{4})(\d{1,2})(\d{1,2})$/); // yyyymmdd-yyyymmdd (DICOM range)
        if (match) {
            return [new Date(match[1], match[2]-1, match[3]), new Date(match[4], match[5]-1, match[6])];
        }
        match = str.match(/^(\d{1,2})[\/\-](\d{1,2})[\/\-](\d{4})$/); // dd/mm/yyyy or dd-mm-yyyy
        if (match) {
            return [new Date(match[3], match[2]-1, match[1]), null];
        }
        match = str.match(/^(\d{4})[\/\-](\d{1,2})[\/\-](\d{1,2})$/); // yyyy/mm/dd or yyyy-mm-dd
        if (match) {
            return [new Date(match[1], match[2]-1, match[3]), null];
        }
        return null;
    }
}
