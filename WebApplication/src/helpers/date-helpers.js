import { parse, format, endOfMonth, endOfYear, startOfMonth, startOfYear, subMonths, subDays, startOfWeek, endOfWeek, subYears } from 'date-fns';

document._datePickerPresetRanges = [
    { tLabel: 'date_picker.today', value: [new Date(), new Date()] },
    { tLabel: 'date_picker.yesterday', value: [subDays(new Date(), 1), subDays(new Date(), 1)] },
    { tLabel: 'date_picker.this_week', value: [startOfWeek(new Date(), { weekStartsOn: 1 }), new Date()] },
    { tLabel: 'date_picker.last_week', value: [startOfWeek(subDays(new Date(), 7), { weekStartsOn: 1 }), endOfWeek(subDays(new Date(), 7), { weekStartsOn: 1 })] },
    { tLabel: 'date_picker.this_month', value: [startOfMonth(new Date()), endOfMonth(new Date())] },
    { tLabel: 'date_picker.last_month', value: [startOfMonth(subMonths(new Date(), 1)), endOfMonth(subMonths(new Date(), 1))] },
    { tLabel: 'date_picker.last_12_months', value: [subYears(new Date(), 1), new Date()] },
];

export default {
    parse(dateStr, format) {
        return parse(dateStr, format, 0);
    },
    toDicomDate(date) {
        return (date.getFullYear() * 10000 + (date.getMonth()+1) * 100 + date.getDate()).toString();
    },
    fromDicomDate(dateStr) {
        let match = null;
        match = dateStr.match(/^(\d{4})(\d{1,2})(\d{1,2})$/); // yyyymmdd (DICOM)
        if (match) {
            return new Date(match[1], match[2]-1, match[3]);
        }
        return null;
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
        // match = str.match(/^(\d{1,2})[\/\-](\d{1,2})[\/\-](\d{4})$/); // dd/mm/yyyy or dd-mm-yyyy
        // if (match) {
        //     return [new Date(match[3], match[2]-1, match[1]), null];
        // }
        // match = str.match(/^(\d{4})[\/\-](\d{1,2})[\/\-](\d{1,2})$/); // yyyy/mm/dd or yyyy-mm-dd
        // if (match) {
        //     return [new Date(match[1], match[2]-1, match[3]), null];
        // }
        return null;
    },
    dicomDateFromDatePicker(dates) {
        let output = "";
        if (dates == null) {
            output = null;
        }
        else if (dates instanceof Date) {
            output = this.toDicomDate(dates);
        }
        else if (dates instanceof Array) {
            if (dates.length == 2 && (dates[0] != null && dates[0].getFullYear != undefined) && (dates[1] != null && dates[1].getFullYear != undefined)) {
                if (this.toDicomDate(dates[0]) == this.toDicomDate(dates[1])) {
                    output = this.toDicomDate(dates[0]);
                } else {
                    output = this.toDicomDate(dates[0]) + "-" + this.toDicomDate(dates[1]);
                }
            } else if (dates.length >= 1 && dates[0].getFullYear != undefined) {
                output = this.toDicomDate(dates[0]);
            } else if (dates.length == 2 && typeof dates[0] == 'string' && typeof dates[1] == 'string') {
                output = dates[0] + "-" + dates[1];
            } else if (dates.length == 1 && typeof dates[0] == 'string') {
                output = dates[0];
            }
        }
        return output;
    },
    formatDateForDisplay(dicomDate, dateFormat) {
        if (!dicomDate) {
            return "";
        }
        if (dicomDate && dicomDate.length == 8) {
            let d = parse(dicomDate, "yyyyMMdd", new Date());
            if (!isNaN(d.getDate())) {
                return format(d, dateFormat);
            }
        }
        if (dicomDate.length > 0) {
            return dicomDate;
        }
        return "";
    },
    isDateTag(tagName) {
        return ["StudyDate", "PatientBirthDate", "SeriesDate", "AcquisitionDate", "ContentDate"].indexOf(tagName) != -1;
    }
}
