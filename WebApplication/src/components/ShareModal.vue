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
            shareLink: ""
        }
    },
    mounted() {
        this.expirationInDays = this.uiOptions.DefaultShareDuration;

        this.$refs['modal-main-div'].addEventListener('show.bs.modal', (e) => {
            // move the modal to body to avoid z-index issues: https://weblog.west-wind.com/posts/2016/sep/14/bootstrap-modal-dialog-showing-under-modal-background
            document.querySelector('body').appendChild(e.target);
        });
    },
    watch: {
        expirationInDays(newValue, oldValue) {
            this.shareLink = ""
        },
    },
    methods: {
        getDurationText(duration) {
            if (duration == 0) {
                return this.$i18n.t('share.never');
            }
            return "" + duration + " " +  this.$i18n.t('share.days');
        },
        async share() {
            let token = await api.createToken({
                tokenType: this.tokens.ShareType,  // defined in configuration file
                resourcesIds: [this.orthancId], 
                level: 'study', 
                validityDuration: this.expirationInDays * 24 * 3600
            })
            this.shareLink = token["Url"];
        },
        copyAndClose() {
            clipboardHelpers.copyToClipboard(this.shareLink);
        }
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            tokens: state => state.configuration.tokens,
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
                    <h5 class="modal-title" id="modalLabel">{{ $t("share.modal_title") + " " + resourceTitle }} </h5>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                </div>
                <div class="modal-body">
                    <div class="container">
                    <div class="row py-3">
                        <div class="col-md-6">
                            {{ $t("share.expires_in") }}
                        </div>
                        <div class="col-md-5">
                            <select class="form-select" v-model="expirationInDays">
                                <option v-for="duration in uiOptions.ShareDurations" :key="duration" :value="duration">
                                    {{ getDurationText(duration) }}</option>
                            </select>
                        </div>
                    </div>
                    <div class="row py-3">
                        <div class="col-md-3">
                            {{ $t("share.link") }}
                        </div>
                        <div class="col-md-8">
                            <div id="parent" style="width:100%">
                                <input type="text" id="txt_input" v-model="shareLink" style="min-width: 100%!important;" />
                            </div>                            
                        </div>
                        <div class="col-md-1">
                            <div class="d-flex flex-row-reverse">
                                <CopyToClipboardButton :valueToCopy="this.shareLink" />
                            </div>
                            </div>
                    </div>
                </div>
                </div>
                <div class="modal-footer">
                    <button type="button" class="btn btn-secondary"
                        data-bs-dismiss="modal">{{ $t("cancel") }}</button>
                    <button v-if="shareLink == ''" type="button" class="btn btn-primary"
                        @click="share()">{{ $t("share.button_title") }}</button>
                    <button v-if="shareLink != ''" type="button" class="btn btn-primary" data-bs-dismiss="modal"
                        @click="copyAndClose()">{{ $t("share.copy_and_close") }}</button>
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