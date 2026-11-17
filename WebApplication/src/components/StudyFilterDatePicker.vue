<script>
import { mapState, mapGetters } from "vuex"
import dateHelpers from "../helpers/date-helpers"
import { isDarkMode } from "../helpers/media-helpers";
import { useIsMobileAny } from "../helpers/media-helpers";

export default {
    props: {
        columnTag: String,
        datePickerUid: String,
        placeholder: String,
        filterClass: String,
    },
    data() {
        return {
            dateValue: null,
            datePickerPresetRanges: document._datePickerPresetRanges,
            isMobileAny: useIsMobileAny(),
        }
    },
    created() {
        this.messageBus.on('language-changed', this.translateDatePicker);
        this.messageBus.on('set-date-picker', this.setDate);
        this.messageBus.on('clear-date-picker', this.clear);
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
        }),
        dateFormat() {
            return this.uiOptions.DateFormat;
        },
        hasRange() {
            return this.columnTag == "StudyDate";
        },
        isDarkMode() {
            // return true;
            return isDarkMode();
        }
    },
    methods: {
        translateDatePicker(languageKey) {
            for (let i in document._datePickerPresetRanges) {
                document._datePickerPresetRanges[i].label = this.$t(document._datePickerPresetRanges[i].tLabel);
            }
        },
        setDate(message) {
            if (message.dateTag == this.columnTag) {
                if (this.columnTag == "StudyDate") {
                    this.dateValue = dateHelpers.parseDateForDatePicker(message.dateValue);
                } else {
                    this.dateValue = dateHelpers.parseDateForDatePicker(message.dateValue)[0];
                }
            }
        },
        clear(message) {
            if (message.dateTag == this.columnTag) {
                this.dateValue = null;
            }
        },
        datePicked(event) {
            this.dateValue = event;
            let dicomNewValue = dateHelpers.dicomDateFromDatePicker(event);
            if (dicomNewValue == null) {
                dicomNewValue = "";
            }

            // console.log("datePicked: ", event);
            this.messageBus.emit("date-picked", {dateTag: this.columnTag, dateValue: dicomNewValue});
        },

    }
}
</script>

<template>
    <template v-if="hasRange">
        <Datepicker key="range" :uid="datePickerUid" :modelValue="dateValue"
            @update:modelValue="datePicked($event)"
            :enable-time-picker="false" :range="true" :preset-dates="datePickerPresetRanges"
            :format="dateFormat" :preview-format="dateFormat" arrow-navigation hide-input-icon text-input
            :highlight="{ weekdays: [6, 0] }" :dark="isDarkMode" :mobile="isMobileAny" :teleport="true">
            <template #yearly="{ label, range, presetDate }">
                <span @click="presetDate(range)">{{ label }}</span>
            </template>
        </Datepicker>
    </template>
    <template v-else>
        <Datepicker key="no-range" :uid="datePickerUid" :modelValue="dateValue"
            @update:modelValue="datePicked($event)"
            :enable-time-picker="false" :range="false"
            :format="dateFormat" :preview-format="dateFormat" arrow-navigation hide-input-icon text-input
            :highlight="{ weekdays: [6, 0] }" :dark="isDarkMode" :mobile="isMobileAny" :teleport="true">
        </Datepicker>
    </template>
</template>