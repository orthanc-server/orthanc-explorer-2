<script>
import { uppie } from "uppie"
import UploadReport from "./UploadReport.vue"
import api from "../orthancApi"

// Drop handler function to get all files
async function getAllFileEntries(dataTransferItemList) {
    let fileEntries = [];
    // Use BFS to traverse entire directory/file structure
    let queue = [];
    // Unfortunately dataTransferItemList is not iterable i.e. no forEach
    for (let i = 0; i < dataTransferItemList.length; i++) {
        queue.push(dataTransferItemList[i].webkitGetAsEntry());
    }
    while (queue.length > 0) {
        let entry = queue.shift();
        if (entry.isFile) {
            fileEntries.push(entry);
        } else if (entry.isDirectory) {
            let reader = entry.createReader();
            queue.push(...await readAllDirectoryEntries(reader));
        }
    }
    return fileEntries;
}

async function getFileFromFileEntry(fileEntry) {
    try {
        return await new Promise((resolve, reject) => fileEntry.file(resolve, reject));
    } catch (err) {
        console.error('getFileFromFileEntry', err);
    }
}

async function getAllFiles(dataTransferItemList) {
    let fileEntries = await getAllFileEntries(dataTransferItemList);

    let files = [];
    for (let fileEntry of fileEntries) {
        files.push(await getFileFromFileEntry(fileEntry));
    }

    return files;
}

// Get all the entries (files or sub-directories) in a directory by calling readEntries until it returns empty array
async function readAllDirectoryEntries(directoryReader) {
    let entries = [];
    let readEntries = await readEntriesPromise(directoryReader);
    while (readEntries.length > 0) {
        entries.push(...readEntries);
        readEntries = await readEntriesPromise(directoryReader);
    }
    return entries;
}

// Wrap readEntries in a promise to make working with readEntries easier
async function readEntriesPromise(directoryReader) {
    try {
        return await new Promise((resolve, reject) => {
            directoryReader.readEntries(resolve, reject);
        });
    } catch (err) {
        console.error('readEntriesPromise', err);
    }
}

function readFileAsync(file) {
    return new Promise((resolve, reject) => {
        let reader = new FileReader();

        reader.onload = () => {
            resolve(reader.result);
        };

        reader.onerror = reject;

        reader.readAsArrayBuffer(file);
    })
}

export default {
    props: ["showStudyDetails", "uploadDisabled", "uploadDisabledMessage", "singleUse", "disableCloseReport"],
    emits: ["uploadCompleted"],
    data() {
        return {
            uploadCounter: 0,
            lastUploadReports: {},
            disabledAfterUpload: false,
            isDragOver: false
        };
    },
    mounted() {
        uppie(document.querySelector("#filesUpload"), this.uppieUploadHandler);
        uppie(document.querySelector("#foldersUpload"), this.uppieUploadHandler);
    },
    methods: {
        onDrop(ev) {
            // console.log("on drop", ev);
            ev.preventDefault();
            this.isDragOver = false;

            getAllFiles(ev.dataTransfer.items).then((files) => {
                this.uploadFiles(files);
            });
        },
        onDragOver(event) {
            event.preventDefault();
            this.isDragOver = true;
        },
        onDragLeave(event) {
            if (!event.currentTarget.contains(event.relatedTarget)) {
                this.isDragOver = false;
            }
        },
        onDeletedUploadReport(uploadReportId) {
            delete this.lastUploadReports[uploadReportId];
        },
        async uploadedFile(uploadId, uploadedFileResponse) {
            let studyId = uploadedFileResponse["ParentStudy"];
            if (!this.lastUploadReports[uploadId].uploadedStudiesIds.has(studyId)) {
                this.lastUploadReports[uploadId].uploadedStudiesIds.add(studyId);

                if (this.showStudyDetails) {
                    const studyResponse = await api.getStudy(studyId);
                    this.lastUploadReports[uploadId].uploadedStudies[studyId] = studyResponse;
                    this.$store.dispatch('studies/addStudy', { study: studyResponse, studyId: studyId, reloadStats: true });
                }
            }
        },
        async uploadFiles(files) {
            files = Array.from(files); // make a copy (https://github.com/orthanc-server/orthanc-explorer-2/issues/98)
            let uploadId = this.uploadCounter++;
            if (this.singleUse) {
                this.disabledAfterUpload = true;
            }

            this.lastUploadReports[uploadId] = {
                id: uploadId,
                filesCount: files.length,
                successFilesCount: 0,
                failedFilesCount: 0,
                skippedFilesCount: 0,
                uploadedStudiesIds: new Set(),
                uploadedStudies: {},  // studies as returned by tools/find
                errorMessages: {}
            };
            for (let file of files) {
                let filename = file.webkitRelativePath || file.name;
                if (file.name == "DICOMDIR") {
                    console.log("upload: skipping DICOMDIR file");
                    this.lastUploadReports[uploadId].skippedFilesCount++;
                    this.lastUploadReports[uploadId].errorMessages[filename] = "skipped";
                    continue;
                }
                const fileContent = await readFileAsync(file);
                try {
                    const uploadResponse = await api.uploadFile(fileContent);

                    if (Array.isArray(uploadResponse)) { // we have uploaded a zip

                        if (uploadResponse.length > 0) {
                            this.lastUploadReports[uploadId].successFilesCount++;
                            for (let uploadFileResponse of uploadResponse) {
                                this.uploadedFile(uploadId, uploadFileResponse);
                            }
                        } else {
                            this.lastUploadReports[uploadId].failedFilesCount++;
                            this.lastUploadReports[uploadId].errorMessages[filename] = "no valid DICOM files found in zip";
                        }
                    } else {
                        this.lastUploadReports[uploadId].successFilesCount++;
                        this.uploadedFile(uploadId, uploadResponse);
                    }
                }
                catch (error) {
                    console.error('uploadFiles', error);
                    let errorMessage = "error " + error.response.status;
                    if (error.response.status >= 400 && error.response.status < 500) {
                        errorMessage = error.response.data.Message;
                    }
                    this.lastUploadReports[uploadId].failedFilesCount++;
                    this.lastUploadReports[uploadId].errorMessages[filename] = errorMessage;
                }
            }

            this.$emit("uploadCompleted", this.lastUploadReports[uploadId].uploadedStudiesIds);
        },
        async uppieUploadHandler(event, formData, files) {
            await this.uploadFiles(event.target.files);

            // reset input for next upload
            if (!this.singleUse) {
                document.getElementById('filesUpload').value = null;
                document.getElementById('foldersUpload').value = null;
            }
        }
    },
    components: { UploadReport }
}
</script>
<template>
    <div>
        <div v-if="!disabledAfterUpload" class="upload-handler-drop-zone" :class="{
            'upload-handler-drop-zone-disabled': uploadDisabled,
            'upload-handler-drop-zone-dragover': isDragOver
        }" @drop="onDrop" @dragover="onDragOver" @dragleave="onDragLeave" :disabled="uploadDisabled">

            <!-- Иконка с анимацией -->
            <div class="upload-icon">
                <i class="fa-solid fa-angles-down"></i>
            </div>

            <!-- Текст -->
            <div v-if="uploadDisabled" class="mb-3">{{ uploadDisabledMessage }}</div>
            <div v-else class="mb-3 upload-text">
                {{ isDragOver ? $t('drop_here') : $t('drop_files') }}
            </div>

            <!-- Кнопки в стиле меню -->
            <div class="upload-menu">
                <label class="menu-item" :class="{ 'menu-item--disabled': uploadDisabled }">
                    <i class="fa-solid fa-folder-open"></i>
                    <span>{{ $t('select_folder') }}</span>
                    <input :disabled="uploadDisabled" type="file" style="display: none;" id="foldersUpload" required
                        multiple directory webkitdirectory allowdirs>
                </label>

                <label class="menu-item" :class="{ 'menu-item--disabled': uploadDisabled }">
                    <i class="fa-solid fa-file-medical"></i>
                    <span>{{ $t('select_files') }}</span>
                    <input :disabled="uploadDisabled" type="file" style="display: none;" id="filesUpload" required
                        multiple>
                </label>
            </div>
        </div>
        <div class="upload-report-list">
            <UploadReport v-for="(upload, key) in lastUploadReports" :report="upload" :key="key"
                :showStudyDetails="showStudyDetails" :disableCloseReport="disableCloseReport"
                @deletedUploadReport="onDeletedUploadReport"></UploadReport>
        </div>
    </div>
</template>
<style scoped>
.upload-report-list {
    display: flex;
    flex-direction: column-reverse;
}

.upload-handler-drop-zone {
    position: relative;
    background: rgba(255, 255, 255, 0.02);
    border-radius: 8px;
    padding: 24px 20px;
    text-align: center;
    margin: 5px 0;
}

.upload-handler-drop-zone::before {
    content: '';
    position: absolute;
    inset: 0;
    border-radius: 8px;
    padding: 1px;
    background: linear-gradient(135deg, rgba(148, 163, 184, 0.3) 0%, rgba(148, 163, 184, 0.1) 100%);
    -webkit-mask: linear-gradient(#fff 0 0) content-box, linear-gradient(#fff 0 0);
    mask: linear-gradient(#fff 0 0) content-box, linear-gradient(#fff 0 0);
    -webkit-mask-composite: xor;
    mask-composite: exclude;
    pointer-events: none;
}

/* При dragover — синий градиент */
.upload-handler-drop-zone-dragover::before {
    background: linear-gradient(135deg, #3b82f6 0%, #06b6d4 100%);
}

.upload-handler-drop-zone-disabled {
    opacity: 0.6;
    border-color: #ff0000d2;
    cursor: not-allowed;
}

/* Иконка */
.upload-icon {
    margin-bottom: 12px;
}

.upload-icon i {
    font-size: 40px;
    color: #94a3b8;
    transition: all 0.2s ease;
}

/* Пульсация при dragover */
.upload-handler-drop-zone-dragover .upload-icon i {
    color: #3b82f6;
    animation: pulse 1s ease infinite;
}

@keyframes pulse {

    0%,
    100% {
        transform: scale(1);
        opacity: 1;
    }

    50% {
        transform: scale(1.1);
        opacity: 0.8;
    }
}

/* Текст */
.upload-text {
    font-size: 13px;
    color: #cbd5e1;
    margin-bottom: 16px;
    line-height: 1.4;
    white-space: pre-line;
}

.upload-handler-drop-zone-dragover .upload-text {
    color: #3b82f6;
    font-weight: 500;
}

/* Подсветка зоны при dragover */
.upload-handler-drop-zone-dragover {
    border-color: #3b82f6 !important;
    background: rgba(59, 130, 246, 0.05);
}

/* === Кнопки в стиле меню === */
.upload-menu {
    display: flex;
    flex-direction: column;
    gap: 4px;
}

.menu-item {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 0px 10px;
    border-radius: 4px;
    cursor: pointer;
    transition: all 0.2s ease;
    color: #94a3b8;
    font-size: 13px;
    font-weight: 400;
}

.menu-item:hover:not(.menu-item--disabled) {
    border-left: 3px solid #8b5cf6;
    background-color: rgba(139, 92, 246, 0.1);
    color: #e2e8f0;

    transform: translateX(4px);
}

.menu-item i {
    width: 20px;
    text-align: center;
    font-size: 14px;
    transition: transform 0.2s ease;
}

.menu-item:hover:not(.menu-item--disabled) i {
    transform: scale(1.1);
    color: #3b82f6;
}

.menu-item--disabled {
    opacity: 0.4;
    cursor: not-allowed;
}
</style>