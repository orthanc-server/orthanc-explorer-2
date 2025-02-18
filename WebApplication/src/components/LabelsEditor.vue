<script>
import { mapState, mapGetters } from "vuex"
import api from "../orthancApi";
import Tags from "bootstrap5-tags/tags.js"
import LabelHelpers from "../helpers/label-helpers"
import { nextTick } from 'vue'


window.filterLabel = (str) => {
    return LabelHelpers.filterLabel(str);
}

export default {
    props: ['labels', 'studyId', 'id', 'title', 'refreshLabelKey'],
    setup() {
    },
    data() {
        return {
            labelsModel: [],
            disableWatcher: false,
            allLabelsLocalCopy: new Set()
        };
    },
    async created() {
        if (this.canCreateNewLabels) {
            // if we can create new labels, we already provide all the existing ones in the list
            this.allLabelsLocalCopy = [...this.allLabels];
        } else {
            // if we can not create new labels, we only provide the available ones
            this.allLabelsLocalCopy = [...this.uiOptions.AvailableLabels];
        }
        this.labelsModel = this.labels;
        // console.log("LabelsEditor: created ", this.labelsModel);
    },
    async mounted() {
        Tags.init();
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            allLabels: state => state.labels.allLabels,
        }),
        getTitle() {
            if (!this.title) {
                return this.$t('labels.study_details_title');
            }
            return this.$t(this.title)
        },
        canCreateNewLabels() {
            return this.uiOptions.AvailableLabels.length == 0; // if there is no AvailableLabels list, this means we can create any new label
        },
        placeHolderText() {
            if (this.canCreateNewLabels) {
                return this.$t('labels.add_labels_placeholder');
            } else {
                return this.$t('labels.add_labels_placeholder_no_create');
            }
        }
    },
    components: {},
    methods: {
        hasLabel(label) {
            // console.log("hasLabel ", label, this.labelsModel.includes(label));
            return this.labelsModel.includes(label);
        },
    },
    watch: {
        async refreshLabelKey(newValue, oldValue) {
            // modifying this value forces refresh but we need to refresh the labelsModel first
            this.disableWatcher = true;
            if (this.canCreateNewLabels) {
                // if we can create new labels, we already provide all the existing ones in the list
                this.allLabelsLocalCopy = [...this.allLabels];
            }
            let labels = await api.getStudyLabels(this.studyId);
            // console.log("LabelsEditor: refreshLabelKey INSIDE 1", labels, this.allLabelsLocalCopy);

            // Don't change the order of these operations !  Or, test with a custom button that adds a label to
            // validate that the LabelsEditor is still updated correctly
            this.labelsModel = [...labels];
            await nextTick();
            this.disableWatcher = false;
            Tags.init();
        },
        labelsModel: {
            async handler(newValue, oldValue) {
                // console.log("LabelsEditor: labelsModel watcher", oldValue, newValue, this.disableWatcher);
                if (this.disableWatcher) {
                    return;
                }
                if (this.studyId) {
                    let changed = await api.updateLabels({
                        studyId: this.studyId,
                        labels: newValue
                    });
                    if (changed) {
                        // this forces refresh of the labels in the StudyItem 
                        this.messageBus.emit("study-labels-updated-in-labels-editor-" + this.studyId);
                        setTimeout(() => { this.$store.dispatch('labels/refresh') }, 100);
                    }
                } else {
                    this.$emit("labelsUpdated", newValue);
                }
            },
            deep: true
        }
    }

}
</script>


<template>
    <div :id="id" :key="refreshLabelKey">
        <label :for="'select-' + id" class="form-label"><span v-html="getTitle"></span><span
                class="invalid-label-tips invalid-label-tips-hidden">{{ $t('labels.valid_alphabet_warning')
                }}</span>
        </label>

        <select class="form-select" :id="'select-' + id" name="tags[]" v-model="labelsModel" multiple
            data-allow-clear="true" :data-allow-new="canCreateNewLabels" data-badge-style="info" data-input-filter="filterLabel"
            :placeholder="placeHolderText">
            <option v-for="label in allLabelsLocalCopy" :key="label" :value="label" :selected="hasLabel(label)">{{ label
                }}
            </option>
        </select>
    </div>
</template>

<style>
.invalid-label-tips {
    color: red;
    font-weight: 600;
    margin-left: 2rem;
}

.invalid-label-tips-hidden {
    display: none;
}
</style>