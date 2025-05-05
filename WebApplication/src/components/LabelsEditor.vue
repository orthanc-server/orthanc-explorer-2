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
    props: ['labels', 'studyId', 'id', 'title', 'showTitle', 'isFilter'],
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
        this.init();
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
            isLabelsLoaded: state => state.labels.loaded
        }),
        isTitleVisible() {
            if (this.showTitle == undefined) {
                return true;
            } else {
                return this.showTitle;
            }
        },
        getTitle() {
            if (!this.title) {
                return this.$t('labels.study_details_title');
            }
            return this.$t(this.title)
        },
        canCreateNewLabels() {
            if (this.isFilter == true) {
                return false;
            }
            return this.uiOptions.AvailableLabels.length == 0; // if there is no AvailableLabels list, this means we can create any new label
        },
        placeHolderText() {
            if (this.isFilter == true) {
                return this.$t('labels.filter_labels_placeholder')
            }

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
        init() {
            // console.log("initializing LabelsEditor ", this.isFilter, this.allLabels, this.uiOptions.AvailableLabels);
            if (this.isFilter) {
                this.allLabelsLocalCopy = [...this.allLabels];
            } else if (this.canCreateNewLabels && this.allLabels) {
                // if we can create new labels, we already provide all the existing ones in the list
                this.allLabelsLocalCopy = [...this.allLabels];
            } else if (!this.canCreateNewLabels && this.uiOptions.AvailableLabels) {
                // if we can not create new labels, we only provide the available ones
                this.allLabelsLocalCopy = [...this.uiOptions.AvailableLabels];
            }
        }
    },
    watch: {
        async isLabelsLoaded(newValue, oldValue) {
            // console.log("labels loaded, init LabelsEditor")
            this.init();            
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
                } else { // bulk-selection or multi-label filter
                    this.$emit("labelsUpdated", newValue);
                }
            },
            deep: true
        }
    }

}
</script>


<template>
    <div :id="id">
        <label v-if="showTitle" :for="'select-' + id" class="form-label"><span v-html="getTitle"></span><span
                class="invalid-label-tips invalid-label-tips-hidden">{{ $t('labels.valid_alphabet_warning')
                }}</span>
        </label>

        <select class="form-select" :id="'select-' + id" name="select-labels" v-model="labelsModel" multiple
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