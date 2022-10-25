<script>
import { mapState } from "vuex"
import CopyToClipboardButton from "./CopyToClipboardButton.vue";
import resourceHelpers from "../helpers/resource-helpers"
import clipboardHelpers from "../helpers/clipboard-helpers"
import api from "../orthancApi"


export default {
    props: ["id", "orthancId", "studyMainDicomTags", "patientMainDicomTags"],
    methods: {
        share() {
        }
    },
    data() {
        return {
            expirationInDays: 0,
            anonymized: "no",
            shareLink: ""
        }
    },
    mounted() {
        console.log("mounted");
        this.expirationInDays = this.uiOptions.DefaultShareDuration;
        this.anonymized = this.uiOptions.AnonymizeSharesByDefault;

        this.$refs['modal-main-div'].addEventListener('show.bs.modal', (e) => {
            // move the modal to body to avoid z-index issues: https://weblog.west-wind.com/posts/2016/sep/14/bootstrap-modal-dialog-showing-under-modal-background
            document.querySelector('body').appendChild(e.target);
        });
    },
    watch: {
        expirationInDays(newValue, oldValue) {
            this.shareLink = ""
        },
        anonymized(newValue, oldValue) {
            this.shareLink = ""
        }
    },
    methods: {
        getDurationText(duration) {
            if (duration == 0) {
                return "never";
            }
            return "" + duration + " days";
        },
        async share() {
            const anonymized = this.anonymized == "yes";
            this.shareLink = await api.shareStudy(this.orthancId, this.studyMainDicomTags["StudyInstanceUID"], anonymized, this.expirationInDays);
        },
        copyAndClose() {
            clipboardHelpers.copyToClipboard(this.shareLink);
        }
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
        }),
        resourceTitle() {
            return resourceHelpers.getResourceTitle("study", this.patientMainDicomTags, this.studyMainDicomTags, null, null);
        }
    },
    components: { CopyToClipboardButton }

};
</script>

<template>
    <div class="modal fade" :id="this.id" tabindex="-1" aria-labelledby="modalLabel" ref="modal-main-div">
        <!-- aria-hidden="true" -->
        <div class="modal-dialog">
            <div class="modal-content">
                <div class="modal-header">
                    <h5 class="modal-title" id="modalLabel">Share {{ resourceTitle }} </h5>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                </div>
                <div class="modal-body">
                    <div class="container">
                    <div class="row py-3">
                        <div class="col-md-6">
                            Expiration
                        </div>
                        <div class="col-md-6">
                            <select class="form-select" v-model="expirationInDays">
                                <option v-for="duration in uiOptions.ShareDurations" :key="duration" :value="duration">
                                    {{ getDurationText(duration) }}</option>
                            </select>
                        </div>
                    </div>
                    <div v-if="uiOptions.EnableAnonymizedShares" class="row py-3">
                        <div class="col-md-6">
                            Anonymize ?
                        </div>
                        <div class="col-md-6">
                            <label class="switch">
                                <input type="checkbox" v-model="anonymized" true-value="yes" false-value="no">
                                <span class="slider round"></span>
                            </label>                            
                        </div>
                    </div>
                    <div class="row py-3">
                        <div class="col-md-3">
                            Link
                        </div>
                        <div class="col-md-9">
                            <div class="float-end">
                            <input type="text" v-model="shareLink">
                            <CopyToClipboardButton :valueToCopy="this.shareLink" />
                        </div>
                        </div>
                    </div>
                </div>
                </div>
                <div class="modal-footer">
                    <button type="button" class="btn btn-secondary"
                        data-bs-dismiss="modal">Cancel</button>
                    <button v-if="shareLink == ''" type="button" class="btn btn-primary"
                        @click="share()">Share</button>
                    <button v-if="shareLink != ''" type="button" class="btn btn-primary" data-bs-dismiss="modal"
                        @click="copyAndClose()">Copy and close</button>
                </div>
            </div>
        </div>
    </div>
</template>

<style>
/* The switch - the box around the slider */
.switch {
  position: relative;
  display: inline-block;
  width: 60px;
  height: 34px;
}

/* Hide default HTML checkbox */
.switch input {
  opacity: 0;
  width: 0;
  height: 0;
}

/* The slider */
.slider {
  position: absolute;
  cursor: pointer;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: #ccc;
  -webkit-transition: .4s;
  transition: .4s;
}

.slider:before {
  position: absolute;
  content: "";
  height: 26px;
  width: 26px;
  left: 4px;
  bottom: 4px;
  background-color: white;
  -webkit-transition: .4s;
  transition: .4s;
}

input:checked + .slider {
  background-color: #2196F3;
}

input:focus + .slider {
  box-shadow: 0 0 1px #2196F3;
}

input:checked + .slider:before {
  -webkit-transform: translateX(26px);
  -ms-transform: translateX(26px);
  transform: translateX(26px);
}

/* Rounded sliders */
.slider.round {
  border-radius: 34px;
}

.slider.round:before {
  border-radius: 50%;
}
</style>