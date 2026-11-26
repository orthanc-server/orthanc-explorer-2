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
        idPrefix: String,
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
        <StudyFilterDatePicker :columnTag="columnTag" :datePickerUid="idPrefix + '-' + columnTag"
            :placeholder="placeholder" :filterClass="filterClass" />
    </div>
    <div v-else>
        <StudyFilterText :columnTag="columnTag"
            :placeholder="placeholder" :filterClass="filterClass" :idPrefix="idPrefix"/>
    </div>
</template>