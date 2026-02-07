<script>
import { mapState } from "vuex";
import api from "../orthancApi";

export default {
    name: "Devices",

    data() {
        return {
            modalities: [],
            isLoadingDevices: false,
            isSavingDevice: false,
            isTesting: null,
            expandedDevice: null,
            showDeviceModal: false,
            editingDevice: null,
            testResults: {},

            deviceForm: {
                id: "",
                AET: "",
                Host: "",
                Port: 104,
                Manufacturer: "Generic",
                AllowEcho: true,
                AllowFind: true,
                AllowFindWorklist: false,
                AllowGet: true,
                AllowMove: true,
                AllowStore: true,
                AllowStorageCommitment: false,
                AllowTranscoding: true,
                UseDicomTls: false,
                Timeout: 0,
                LocalAet: ""
            },

            manufacturers: [
                "Generic",
                "GenericNoWildcardInDates",
                "GenericNoUniversalWildcard",
                "StoreScp",
                "GE",
                "Vitrea",
                "Agfa"
            ]
        };
    },

    computed: {
        ...mapState("configuration", ["userProfile", "system"]),

        canManageDevices() {
            const perms = this.userProfile?.permissions || [];
            return perms.includes('settings') || perms.includes('admin-permissions') || perms.includes('all');
        }
    },

    async mounted() {
        await this.loadDevices();
    },

    methods: {
        async loadDevices() {
            this.isLoadingDevices = true;
            try {
                const response = await api.getModalities();
                this.modalities = Object.entries(response || {}).map(([id, config]) => ({
                    id,
                    ...config
                }));
            } catch (error) {
                console.error("Failed to load devices:", error);
            } finally {
                this.isLoadingDevices = false;
            }
        },

        toggleExpand(deviceId) {
            this.expandedDevice = this.expandedDevice === deviceId ? null : deviceId;
        },

        openDeviceModal(device = null) {
            this.editingDevice = device;
            if (device) {
                this.deviceForm = {
                    id: device.id,
                    AET: device.AET || "",
                    Host: device.Host || "",
                    Port: device.Port || 104,
                    Manufacturer: device.Manufacturer || "Generic",
                    AllowEcho: device.AllowEcho !== false,
                    AllowFind: device.AllowFind !== false,
                    AllowFindWorklist: device.AllowFindWorklist === true,
                    AllowGet: device.AllowGet !== false,
                    AllowMove: device.AllowMove !== false,
                    AllowStore: device.AllowStore !== false,
                    AllowStorageCommitment: device.AllowStorageCommitment === true,
                    AllowTranscoding: device.AllowTranscoding !== false,
                    UseDicomTls: device.UseDicomTls === true,
                    Timeout: device.Timeout || 0,
                    LocalAet: device.LocalAet || ""
                };
            } else {
                this.deviceForm = {
                    id: "",
                    AET: "",
                    Host: "",
                    Port: 104,
                    Manufacturer: "Generic",
                    AllowEcho: true,
                    AllowFind: true,
                    AllowFindWorklist: false,
                    AllowGet: true,
                    AllowMove: true,
                    AllowStore: true,
                    AllowStorageCommitment: false,
                    AllowTranscoding: true,
                    UseDicomTls: false,
                    Timeout: 0,
                    LocalAet: ""
                };
            }
            this.showDeviceModal = true;
        },

        async saveDevice() {
            if (!this.deviceForm.id || !this.deviceForm.AET || !this.deviceForm.Host) {
                alert(this.$t('devices.validation_required'));
                return;
            }

            const payload = {
                AET: this.deviceForm.AET,
                Host: this.deviceForm.Host,
                Port: parseInt(this.deviceForm.Port),
                Manufacturer: this.deviceForm.Manufacturer,
                AllowEcho: this.deviceForm.AllowEcho,
                AllowFind: this.deviceForm.AllowFind,
                AllowFindWorklist: this.deviceForm.AllowFindWorklist,
                AllowGet: this.deviceForm.AllowGet,
                AllowMove: this.deviceForm.AllowMove,
                AllowStore: this.deviceForm.AllowStore,
                AllowStorageCommitment: this.deviceForm.AllowStorageCommitment,
                AllowTranscoding: this.deviceForm.AllowTranscoding,
                UseDicomTls: this.deviceForm.UseDicomTls,
                Timeout: parseInt(this.deviceForm.Timeout) || 0
            };

            if (this.deviceForm.LocalAet) {
                payload.LocalAet = this.deviceForm.LocalAet;
            }

            this.isSavingDevice = true;
            try {
                await api.createOrUpdateModality(this.deviceForm.id, payload);
                this.showDeviceModal = false;
                await this.loadDevices();
                this.$toast?.success(this.editingDevice ? this.$t('devices.updated') : this.$t('devices.created'));
            } catch (error) {
                alert(this.$t('devices.save_error') + ": " + error.message);
            } finally {
                this.isSavingDevice = false;
            }
        },

        async deleteDevice(id) {
            if (!confirm(this.$t('devices.confirm_delete', { id }))) return;
            try {
                await api.deleteModality(id);
                await this.loadDevices();
                this.$toast?.success(this.$t('devices.deleted'));
            } catch (error) {
                alert(this.$t('devices.delete_error') + ": " + error.message);
            }
        },

        async testDevice(id) {
            this.isTesting = id;
            try {
                const result = await api.testModalityEcho(id);
                this.testResults = { ...this.testResults, [id]: { success: true, data: result } };
                this.$toast?.success(this.$t('devices.test_success'));
            } catch (error) {
                this.testResults = { ...this.testResults, [id]: { success: false, error: error.message } };
                this.$toast?.error(this.$t('devices.test_failed'));
            } finally {
                this.isTesting = null;
            }
        }
    }
};
</script>

<template>
    <div class="devices-section">
        <!-- Header с кнопкой Add -->
        <div class="section-header">
            <div class="header-left">
                <i class="bi bi-hdd-network-fill"></i>
                <h3>{{ $t('settings.devices.title') }}</h3>
                <span v-if="modalities.length > 0" class="badge-count">{{ modalities.length }}</span>
            </div>
            <button v-if="canManageDevices" class="btn btn-primary btn-sm" @click="openDeviceModal()">
                <i class="bi bi-plus-lg"></i>
                {{ $t('devices.add') }}
            </button>
        </div>

        <!-- Loading -->
        <div v-if="isLoadingDevices" class="loading-state">
            <i class="bi bi-arrow-repeat spin"></i>
            <span>{{ $t('loading') }}</span>
        </div>

        <!-- Empty -->
        <div v-else-if="modalities.length === 0" class="empty-state">
            <i class="bi bi-hdd-network"></i>
            <p>{{ $t('devices.empty') }}</p>
            <button v-if="canManageDevices" class="btn btn-primary" @click="openDeviceModal()">
                <i class="bi bi-plus-lg"></i>
                {{ $t('devices.add_first') }}
            </button>
        </div>

        <!-- List -->
        <div v-else class="devices-list">
            <div v-for="mod in modalities" :key="mod.id" class="device-item"
                :class="{ expanded: expandedDevice === mod.id }">
                <!-- Header -->
                <div class="device-header" @click="toggleExpand(mod.id)">
                    <div class="device-main">
                        <div class="device-icon">
                            <i class="bi bi-router"></i>
                        </div>
                        <div class="device-info">
                            <h4 class="device-name cut-text">{{ mod.id }}</h4>
                            <span class="device-aet">{{ mod.AET }}</span>
                        </div>
                        <div v-if="testResults[mod.id]" class="test-indicator"
                            :class="{ success: testResults[mod.id].success }">
                            <i
                                :class="testResults[mod.id].success ? 'bi bi-check-circle-fill' : 'bi bi-x-circle-fill'"></i>
                        </div>
                    </div>

                    <div class="device-actions-compact">
                        <button class="btn-icon btn-test" @click.stop="testDevice(mod.id)"
                            :disabled="isTesting === mod.id" :title="$t('devices.test')">
                            <i :class="isTesting === mod.id ? 'bi bi-arrow-repeat spin' : 'bi bi-broadcast'"></i>
                        </button>
                        <button class="btn-icon btn-edit" @click.stop="openDeviceModal(mod)" :title="$t('edit')">
                            <i class="bi bi-pencil"></i>
                        </button>
                        <button class="btn-icon btn-delete" @click.stop="deleteDevice(mod.id)" :title="$t('delete')">
                            <i class="bi bi-trash"></i>
                        </button>
                        <button class="btn-icon btn-expand" :title="$t('devices.expand')">
                            <i class="bi" :class="expandedDevice === mod.id ? 'bi-chevron-up' : 'bi-chevron-down'"></i>
                        </button>
                    </div>
                </div>

                <!-- Expanded Details -->
                <div v-if="expandedDevice === mod.id" class="device-details">
                    <div class="details-grid">
                        <div class="detail-item">
                            <span class="detail-label">{{ $t('devices.host') }}</span>
                            <span class="detail-value">{{ mod.Host }}:{{ mod.Port }}</span>
                        </div>
                        <div class="detail-item">
                            <span class="detail-label">{{ $t('devices.manufacturer') }}</span>
                            <span class="detail-value">{{ mod.Manufacturer }}</span>
                        </div>
                        <div class="detail-item" v-if="mod.LocalAet">
                            <span class="detail-label">{{ $t('devices.local_aet') }}</span>
                            <span class="detail-value">{{ mod.LocalAet }}</span>
                        </div>
                    </div>

                    <div class="capabilities-row">
                        <span class="cap-label">{{ $t('devices.capabilities') }}:</span>
                        <div class="cap-badges">
                            <span v-if="mod.AllowEcho" class="cap-badge" title="C-ECHO">Echo</span>
                            <span v-if="mod.AllowStore" class="cap-badge store" title="C-STORE">Store</span>
                            <span v-if="mod.AllowFind" class="cap-badge find" title="C-FIND">Find</span>
                            <span v-if="mod.AllowGet" class="cap-badge get" title="C-GET">Get</span>
                            <span v-if="mod.AllowMove" class="cap-badge move" title="C-MOVE">Move</span>
                            <span v-if="mod.AllowFindWorklist" class="cap-badge worklist" title="Worklist">WL</span>
                            <span v-if="mod.UseDicomTls" class="cap-badge tls" title="DICOM TLS">TLS</span>
                        </div>
                    </div>

                    <div v-if="testResults[mod.id] && !testResults[mod.id].success" class="test-error">
                        <i class="bi bi-exclamation-triangle"></i>
                        <span>{{ testResults[mod.id].error || $t('devices.test_failed') }}</span>
                    </div>
                </div>
            </div>
        </div>

        <!-- Modal -->
        <Teleport to="body">
            <div v-if="showDeviceModal" class="modal-overlay" @click.self="showDeviceModal = false">
                <div class="modal-content modal-sm">
                    <div class="modal-header">
                        <h3>{{ editingDevice ? $t('devices.edit_title') : $t('devices.new_title') }}</h3>
                        <button class="btn-close" @click="showDeviceModal = false">
                            <i class="bi bi-x-lg"></i>
                        </button>
                    </div>

                    <div class="modal-body">
                        <div class="form-row">
                            <div class="form-group">
                                <label>ID *</label>
                                <input v-model="deviceForm.id" :disabled="editingDevice" placeholder="MY_MODALITY" />
                            </div>
                            <div class="form-group">
                                <label>AET *</label>
                                <input v-model="deviceForm.AET" placeholder="PACS_SERVER" />
                            </div>
                        </div>

                        <div class="form-row">
                            <div class="form-group">
                                <label>Host *</label>
                                <input v-model="deviceForm.Host" placeholder="192.168.1.100" />
                            </div>
                            <div class="form-group">
                                <label>Port *</label>
                                <input v-model.number="deviceForm.Port" type="number" min="1" max="65535" />
                            </div>
                        </div>

                        <div class="form-group">
                            <label>Manufacturer</label>
                            <select v-model="deviceForm.Manufacturer">
                                <option v-for="m in manufacturers" :key="m" :value="m">{{ m }}</option>
                            </select>
                        </div>

                        <div class="form-group">
                            <label>Local AET (optional)</label>
                            <input v-model="deviceForm.LocalAet" :placeholder="system?.DicomAet" />
                        </div>

                        <div class="capabilities-form">
                            <label>{{ $t('devices.capabilities') }}</label>
                            <div class="checkbox-grid">
                                <label class="checkbox-item">
                                    <input type="checkbox" v-model="deviceForm.AllowEcho" />
                                    <span>C-ECHO</span>
                                </label>
                                <label class="checkbox-item">
                                    <input type="checkbox" v-model="deviceForm.AllowStore" />
                                    <span>C-STORE</span>
                                </label>
                                <label class="checkbox-item">
                                    <input type="checkbox" v-model="deviceForm.AllowFind" />
                                    <span>C-FIND</span>
                                </label>
                                <label class="checkbox-item">
                                    <input type="checkbox" v-model="deviceForm.AllowGet" />
                                    <span>C-GET</span>
                                </label>
                                <label class="checkbox-item">
                                    <input type="checkbox" v-model="deviceForm.AllowMove" />
                                    <span>C-MOVE</span>
                                </label>
                                <label class="checkbox-item">
                                    <input type="checkbox" v-model="deviceForm.AllowFindWorklist" />
                                    <span>Worklist</span>
                                </label>
                                <label class="checkbox-item">
                                    <input type="checkbox" v-model="deviceForm.AllowStorageCommitment" />
                                    <span>Storage Commit.</span>
                                </label>
                                <label class="checkbox-item">
                                    <input type="checkbox" v-model="deviceForm.AllowTranscoding" />
                                    <span>Transcoding</span>
                                </label>
                                <label class="checkbox-item">
                                    <input type="checkbox" v-model="deviceForm.UseDicomTls" />
                                    <span>DICOM TLS</span>
                                </label>
                            </div>
                        </div>
                    </div>

                    <div class="modal-footer">
                        <button class="btn btn-secondary" @click="showDeviceModal = false">
                            {{ $t('cancel') }}
                        </button>
                        <button class="btn btn-primary" @click="saveDevice" :disabled="isSavingDevice">
                            <i :class="isSavingDevice ? 'bi bi-arrow-repeat spin' : 'bi bi-check-lg'"></i>
                            {{ isSavingDevice ? $t('saving') : $t('save') }}
                        </button>
                    </div>
                </div>
            </div>
        </Teleport>
    </div>
</template>

<style scoped>
.devices-section {
    width: 100%;
}

.section-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 20px;
}

.header-left {
    display: flex;
    align-items: center;
    gap: 12px;
}

.header-left i {
    font-size: 20px;
    color: #3b82f6;
}

.header-left h3 {
    margin: 0;
    font-size: 16px;
    font-weight: 600;
    color: #334155;
}

.badge-count {
    background: #e0e7ff;
    color: #3730a3;
    padding: 2px 10px;
    border-radius: 12px;
    font-size: 13px;
    font-weight: 600;
}

.btn {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    padding: 8px 16px;
    border-radius: 8px;
    font-size: 14px;
    font-weight: 500;
    cursor: pointer;
    border: none;
    transition: all 0.2s;
}

.btn-primary {
    background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
    color: white;
}

.btn-primary:hover:not(:disabled) {
    background: linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%);
    transform: translateY(-1px);
    box-shadow: 0 4px 12px rgba(59, 130, 246, 0.3);
}

.btn-sm {
    padding: 6px 12px;
    font-size: 13px;
}

.loading-state,
.empty-state {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    padding: 40px;
    color: #64748b;
    gap: 12px;
}

.empty-state i {
    font-size: 48px;
    color: #cbd5e1;
}

.devices-list {
    display: flex;
    flex-direction: column;
    gap: 8px;
}

.device-item {
    background: white;
    border-radius: 10px;
    border: 1px solid #e2e8f0;
    overflow: hidden;
    transition: all 0.2s;
}

.device-item:hover {
    border-color: #cbd5e1;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.05);
}

.device-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 14px 16px;
    cursor: pointer;
    user-select: none;
}

.device-main {
    display: flex;
    align-items: center;
    gap: 12px;
    flex: 1;
}

.device-icon {
    width: 36px;
    height: 36px;
    background: linear-gradient(135deg, #3b82f6, #8b5cf6);
    border-radius: 8px;
    display: flex;
    align-items: center;
    justify-content: center;
    color: white;
    font-size: 16px;
}

.device-info {
    display: flex;
    flex-direction: column;
    gap: 2px;
    max-width: 160px;
}

.device-name {
    font-size: 14px;
    font-weight: 600;
    color: #1e293b;
    margin: 0;
}

.device-aet {
    font-size: 12px;
    color: #64748b;
    font-family: monospace;
}

.test-indicator {
    margin-left: 8px;
    font-size: 16px;
}

.test-indicator.success {
    color: #22c55e;
}

.test-indicator:not(.success) {
    color: #ef4444;
}

.device-actions-compact {
    display: flex;
    gap: 4px;
}

.btn-icon {
    width: 32px;
    height: 32px;
    border-radius: 6px;
    border: none;
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    transition: all 0.2s;
    background: transparent;
    color: #64748b;
    font-size: 14px;
}

.btn-icon:hover {
    background: #f1f5f9;
}

.btn-test:hover {
    background: #dbeafe;
    color: #1e40af;
}

.btn-edit:hover {
    background: #f3e8ff;
    color: #6b21a8;
}

.btn-delete:hover {
    background: #fee2e2;
    color: #991b1b;
}

.device-details {
    padding: 0 16px 16px 64px;
    background: #fafafa;
    border-top: 1px solid #f1f5f9;
}

.details-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
    gap: 12px;
    padding: 16px 0;
}

.detail-item {
    display: flex;
    flex-direction: column;
    gap: 4px;
}

.detail-label {
    font-size: 11px;
    color: #94a3b8;
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.detail-value {
    font-size: 13px;
    color: #1e293b;
    font-weight: 500;
}

.capabilities-row {
    display: flex;
    align-items: center;
    gap: 12px;
    flex-wrap: wrap;
    padding-bottom: 16px;
}

.cap-label {
    font-size: 12px;
    color: #64748b;
}

.cap-badges {
    display: flex;
    gap: 6px;
    flex-wrap: wrap;
}

.cap-badge {
    padding: 3px 10px;
    border-radius: 20px;
    font-size: 11px;
    font-weight: 500;
    background: #e2e8f0;
    color: #475569;
}

.cap-badge.store {
    background: #dcfce7;
    color: #166534;
}

.cap-badge.find {
    background: #dbeafe;
    color: #1e40af;
}

.cap-badge.get {
    background: #f3e8ff;
    color: #6b21a8;
}

.cap-badge.move {
    background: #fef3c7;
    color: #92400e;
}

.cap-badge.worklist {
    background: #fce7f3;
    color: #9d174d;
}

.cap-badge.tls {
    background: #ccfbf1;
    color: #0f766e;
}

.test-error {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 10px 14px;
    background: #fee2e2;
    border-radius: 8px;
    color: #991b1b;
    font-size: 13px;
    margin-bottom: 16px;
}

.spin {
    animation: spin 1s linear infinite;
}

@keyframes spin {
    from {
        transform: rotate(0deg);
    }

    to {
        transform: rotate(360deg);
    }
}

.modal-overlay {
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background: rgba(0, 0, 0, 0.5);
    display: flex;
    align-items: center;
    justify-content: center;
    z-index: 1000;
    padding: 20px;
}

.modal-content {
    background: white;
    border-radius: 12px;
    width: 100%;
    max-width: 480px;
    max-height: 90vh;
    overflow: hidden;
    display: flex;
    flex-direction: column;
}

.modal-sm {
    max-width: 480px;
}

.modal-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 16px 20px;
    border-bottom: 1px solid #e2e8f0;
}

.modal-header h3 {
    margin: 0;
    font-size: 16px;
    font-weight: 600;
    color: #1e293b;
}

.btn-close {
    background: none;
    border: none;
    font-size: 18px;
    color: #64748b;
    cursor: pointer;
}

.modal-body {
    padding: 20px;
    overflow-y: auto;
}

.form-row {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 16px;
    margin-bottom: 16px;
}

.form-group {
    margin-bottom: 16px;
}

.form-group label {
    display: block;
    margin-bottom: 6px;
    font-size: 12px;
    font-weight: 500;
    color: #374151;
}

.form-group input,
.form-group select {
    width: 100%;
    padding: 10px 12px;
    border: 1px solid #d1d5db;
    border-radius: 8px;
    font-size: 14px;
    transition: all 0.2s;
}

.form-group input:focus,
.form-group select:focus {
    outline: none;
    border-color: #3b82f6;
    box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);
}

.capabilities-form {
    margin-top: 8px;
}

.capabilities-form>label {
    display: block;
    margin-bottom: 12px;
    font-size: 12px;
    font-weight: 500;
    color: #374151;
}

.checkbox-grid {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 10px;
}

.checkbox-item {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px;
    background: #f8fafc;
    border-radius: 6px;
    cursor: pointer;
    font-size: 13px;
    transition: background 0.2s;
}

.checkbox-item:hover {
    background: #f1f5f9;
}

.checkbox-item input {
    width: 16px;
    height: 16px;
    accent-color: #3b82f6;
}

.modal-footer {
    display: flex;
    justify-content: flex-end;
    gap: 12px;
    padding: 16px 20px;
    border-top: 1px solid #e2e8f0;
    background: #f9fafb;
}

.btn-secondary {
    background: #f1f5f9;
    color: #475569;
}

.btn-secondary:hover {
    background: #e2e8f0;
}

@media (max-width: 640px) {
    .form-row {
        grid-template-columns: 1fr;
    }

    .checkbox-grid {
        grid-template-columns: repeat(2, 1fr);
    }

    .device-header {
        gap: 12px;
    }

    .device-actions-compact {
        width: 100%;
        justify-content: flex-end;
    }

    .device-details {
        padding-left: 16px;
    }

    .section-header {
        flex-wrap: wrap;
        gap: 12px;
    }
}
</style>