<script>
import InstanceDetails from "./InstanceDetails.vue";
import bootstrap from "bootstrap/dist/js/bootstrap.bundle.min.js"
import SelectionStatus from "../helpers/selection-status";
import SourceType from '../helpers/source-type';
import { mapState, mapGetters } from "vuex"
import { useIsMobileAny, useIsMobileNarrow, useIsMobileWide } from "../helpers/media-helpers";

const expandedInstances = new Set();


export default {
    props: ["instanceId", "instanceInfo", 'seriesMainDicomTags', 'studyMainDicomTags', 'patientMainDicomTags', 'studyId', 'seriesId', 'studySeries', 'seriesInstances'],
    emits: ['deletedInstance'],
    data() {
        return {
            loaded: false,
            expanded: expandedInstances.has(this.instanceId),
            isMobileAny: useIsMobileAny(),
            isMobileNarrow: useIsMobileNarrow(),
            isMobileWide: useIsMobileWide(),
        };
    },
    mounted() {
        for (const [k, v] of Object.entries(this.$route.query)) {
            if (k === 'expand') {
                if (v === 'instance') {
                    this.expanded = true;
                }
            }
        }
    },
    computed: {
        ...mapState({
            studiesSourceType: state => state.studies.sourceType,
        }),
        isTableMode() {
            return !this.isMobileAny || this.isMobileWide;
        },
        isMultipleSelectionEnabled() {
            return this.studiesSourceType != SourceType.REMOTE_DICOM;
        },
        isSelected() {
            return this.$store.getters['selection/getInstanceSelectionStatus'](this.studyId, this.seriesId, this.instanceId) != SelectionStatus.NOT_SELECTED;
        },
        isPartialySelected() {
            return this.$store.getters['selection/getInstanceSelectionStatus'](this.studyId, this.seriesId, this.instanceId) == SelectionStatus.PARTIAL;
        }
    },
    methods: {
        onDeletedInstance(instanceId) {
            this.$emit('deletedInstance', instanceId);
        },
        async clickedSelect() {
            await this.$store.dispatch('selection/selectInstance', { studyId: this.studyId, seriesId: this.seriesId, studySeries: this.studySeries, instanceId: this.instanceId, seriesInstances: this.seriesInstances, isSelected: !this.isSelected }); // this.selected is the value before the click
        }
    },
    watch: {
        // keep the expanded state in a module-level set so it survives
        // the unmount/remount that happens when switching view mode
        expanded(value) {
            if (value) {
                expandedInstances.add(this.instanceId);
            } else {
                expandedInstances.delete(this.instanceId);
            }
        }
    },
    components: { InstanceDetails }
}
</script>


<template>
    <div style="display: contents">
        <tbody v-if="isTableMode">
            <tr :class="{ 'instance-row-expanded': expanded }">
                <td>
                    <div v-if="isMultipleSelectionEnabled" class="form-check">
                        <input class="form-check-input" type="checkbox" :checked="isSelected"
                            :indeterminate="isPartialySelected" @click="clickedSelect">
                    </div>
                </td>
                <td class="cut-text text-center" @click="expanded = !expanded">{{
                        instanceInfo.MainDicomTags.InstanceNumber }}</td>
                <td class="cut-text" @click="expanded = !expanded">{{
                        instanceInfo.MainDicomTags.SOPInstanceUID }}</td>
                <td class="cut-text text-center" @click="expanded = !expanded">{{
                        instanceInfo.MainDicomTags.NumberOfFrames }}</td>
            </tr>
            <tr v-if="expanded" class="instance-details-expanded">
                <td colspan="100">
                    <InstanceDetails v-if="this.expanded" :instanceId="this.instanceId"
                        :studyMainDicomTags="this.studyMainDicomTags" :seriesMainDicomTags="this.seriesMainDicomTags"
                        :patientMainDicomTags="this.patientMainDicomTags" @deletedInstance="onDeletedInstance">
                    </InstanceDetails>
                </td>
            </tr>
        </tbody>
        <div v-else-if="isMobileNarrow" class="mobile-instances-card d-flex flex-column gap-1"
            :class="{ 'expanded': expanded, 'selected': isSelected }">
            <div class="resource-card card mb-2" @click="expanded = !expanded" style="cursor: pointer">
                <div class="card-body p-0">
                    <div class="d-flex">
                        <div
                            class="resource-card-icons-small d-flex flex-row justify-content-around align-items-center py-2">
                            <input class="form-check-input" type="checkbox" :checked="isSelected"
                                :indeterminate="isPartialySelected" @click.stop="clickedSelect">
                        </div>
                        <div class="resource-card-info flex-grow-1">
                            <div class="resource-card-line-1">
                                <div class="field-w-20">{{ instanceInfo.MainDicomTags.InstanceNumber }}</div>
                                <div class="field-w-70">{{ instanceInfo.MainDicomTags.SOPInstanceUID }}</div>
                                <div class="field-w-10">{{ instanceInfo.MainDicomTags.NumberOfFrames }}</div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
            <div v-if="expanded" class="border-top p-2">
                <InstanceDetails v-if="this.expanded" :instanceId="this.instanceId"
                    :studyMainDicomTags="this.studyMainDicomTags" :seriesMainDicomTags="this.seriesMainDicomTags"
                    :patientMainDicomTags="this.patientMainDicomTags" @deletedInstance="onDeletedInstance">
                </InstanceDetails>
            </div>
        </div>
    </div>
</template>

<style scoped>
.instance-row-expanded {
    background-color: var(--instance-details-bg-color);
    font-weight: 700;

    border-top: 3px !important;
    border-style: solid !important;
    border-color: black !important;
}

.instance-row-expanded>:first-child {
    border-bottom: 0px !important;
}


.instance-details-expanded {
    background-color: var(--instance-details-bg-color);

    border-top: 0px !important;
    border-bottom: 3px !important;
    border-style: solid !important;
    border-color: black !important;
}
</style>