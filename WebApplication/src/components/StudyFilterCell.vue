<script>
import { useIsMobileAny, useIsMobileWide } from "../helpers/media-helpers";

import StudyFilterModalityDropdown from "./StudyFilterModalityDropdown.vue"
import StudyFilterDatePicker from "./StudyFilterDatePicker.vue"
import StudyFilterText from "./StudyFilterText.vue"

export default {
    props: {
        columnTag: String,
        placeholder: String,
        filterClass: String,
        allModalitiesSelected: Boolean,
        noneModalitiesSelected: Boolean,
        showModalityDropdown: Boolean,
        modalitiesFilter: Array,
        inputClass: { default: "form-control study-list-filter" },
        idPrefix: String,
        // values from StudyList
        // textValue: String,
    },
    data() {
        return {
            isMobileAny: useIsMobileAny(),
            isMobileWide: useIsMobileWide(),
        }
    },
    emits: ['update:textValue'],
    methods: {
    },
    components: { StudyFilterModalityDropdown, StudyFilterText, StudyFilterDatePicker }

}
</script>

<template>
    <div v-if="columnTag == 'modalities'" class="dropdown">
        <StudyFilterModalityDropdown :buttonId="idPrefix + '-modalities-button'" />
    </div>
    <div v-else-if="columnTag == 'StudyDate' || columnTag == 'PatientBirthDate'">
        <StudyFilterDatePicker :columnTag="columnTag" :datePickerUid="idPrefix + '-study-date'"
            :placeholder="placeholder" :filterClass="filterClass" />
    </div>
    <div v-else>
        <StudyFilterText :columnTag="columnTag"
            :placeholder="placeholder" :filterClass="filterClass"
            :inputClass="isMobileAny && !isMobileWide ? 'form-control study-list-filter w-100 py-1' : 'form-control study-list-filter'" />
    </div>
</template>