<script>
import { mapState } from "vuex";
import Datepicker from "@vuepic/vue-datepicker";
import dateHelpers from "../helpers/date-helpers";

export default {
  name: "MobileFiltersModal",
  components: {
    Datepicker,
  },
  props: {
    currentStudyDate: { type: String, default: "" },
    visible: {
      type: Boolean,
      default: false,
    },
  },
  emits: ["close", "apply-filters"],
  data() {
    return {
      filters: {
        PatientName: "",
        PatientID: "",
        AccessionNumber: "",
        StudyDate: null,
        PatientBirthDate: null,
        StudyDescription: "",
        ModalitiesInStudy: {},
        labels: [],
      },
      studyDateRange: null,
      birthDateRange: null,
      // 🟡 Drag state с поддержкой mouse
      isDragging: false,
      startY: 0,
      currentY: 0,
      modalHeight: 75, // процент от высоты экрана
      // 🟡 Реактивный стиль вместо прямой манипуляции DOM
      modalContentStyle: {},
      // 🔴 Сохранение предыдущего overflow
      previousBodyOverflow: "",
      // 🟡 Флаги защиты
      isApplying: false,
      isLoadingFilters: false,
    };
  },
  computed: {
    ...mapState({
      uiOptions: (state) => state.configuration.uiOptions,
      allLabels: (state) => state.labels.allLabels,
      dicomTagsFilters: (state) => state.studies.dicomTagsFilters,
      labelFilters: (state) => state.studies.labelFilters,
    }),
    modalitiesList() {
      return this.uiOptions?.ModalitiesFilter || ["CT", "MR", "US", "XR", "CR", "DX", "MG", "PT", "NM"];
    },
    selectedModalitiesCount() {
      return Object.values(this.filters.ModalitiesInStudy).filter(v => v).length;
    },
    hasAnyFilter() {
      if (this.filters.PatientName) return true;
      if (this.filters.PatientID) return true;
      if (this.filters.AccessionNumber) return true;
      if (this.filters.StudyDescription) return true;
      if (this.studyDateRange && (Array.isArray(this.studyDateRange) ? this.studyDateRange[0] : this.studyDateRange)) return true;
      if (this.birthDateRange && (Array.isArray(this.birthDateRange) ? this.birthDateRange[0] : this.birthDateRange)) return true;
      if (this.selectedModalitiesCount > 0 && this.selectedModalitiesCount < this.modalitiesList.length) return true;
      if (this.filters.labels.length > 0) return true;
      return false;
    },
    datePickerFormat() {
      return this.uiOptions?.DateFormat || "dd.MM.yyyy";
    },
    isDarkMode() {
      return true;
    },
    // 🟡 Реактивный стиль для модалки
    computedModalStyle() {
      if (this.modalContentStyle.height) {
        return this.modalContentStyle;
      }
      return { height: `${this.modalHeight}vh` };
    },
  },
  watch: {
    visible: {
      immediate: true,
      handler(newVal) {
        if (newVal) {
          this.loadCurrentFilters();
          this.lockBodyScroll();
        } else {
          this.unlockBodyScroll();
        }
      },
    },
    // 🟡 Отслеживаем изменение списка модальностей
    modalitiesList: {
      handler(newList, oldList) {
        if (JSON.stringify(newList) !== JSON.stringify(oldList)) {
          this.initModalities();
        }
      },
    },
  },
  mounted() {
    this.initModalities();
    // 🟡 Обработка Escape
    document.addEventListener("keydown", this.handleKeydown);
  },
  beforeUnmount() {
    // 🔴 Гарантированная очистка при уничтожении компонента
    this.unlockBodyScroll();
    document.removeEventListener("keydown", this.handleKeydown);
  },
  methods: {
    // 🔴 Блокировка скролла с сохранением предыдущего значения
    lockBodyScroll() {
      this.previousBodyOverflow = document.body.style.overflow;
      document.body.style.overflow = "hidden";
    },
    unlockBodyScroll() {
      document.body.style.overflow = this.previousBodyOverflow;
    },
    // 🟡 Обработка Escape
    handleKeydown(e) {
      if (e.key === "Escape" && this.visible) {
        this.close();
      }
    },
    initModalities() {
      const newModalities = {};
      for (const mod of this.modalitiesList) {
        newModalities[mod] = true;
      }
      this.filters.ModalitiesInStudy = newModalities;
    },
    async loadCurrentFilters() {
      if (this.isLoadingFilters) return;
      this.isLoadingFilters = true;

      try {
        // Загружаем текущие фильтры из store
        this.filters.PatientName = this.dicomTagsFilters?.PatientName || "";
        this.filters.PatientID = this.dicomTagsFilters?.PatientID || "";
        this.filters.AccessionNumber = this.dicomTagsFilters?.AccessionNumber || "";
        this.filters.StudyDescription = this.dicomTagsFilters?.StudyDescription || "";

        // Даты с обработкой ошибок
        try {
          if (this.currentStudyDate) {
            this.studyDateRange = dateHelpers.parseDateForDatePicker(this.currentStudyDate);
          } else {
            this.studyDateRange = null;
          }
        } catch (e) {
          console.warn("Failed to parse StudyDate:", e);
          this.studyDateRange = null;
        }

        try {
          if (this.dicomTagsFilters?.PatientBirthDate) {
            this.birthDateRange = dateHelpers.parseDateForDatePicker(this.dicomTagsFilters.PatientBirthDate);
          } else {
            this.birthDateRange = null;
          }
        } catch (e) {
          console.warn("Failed to parse PatientBirthDate:", e);
          this.birthDateRange = null;
        }

        // Модальности
        const currentModalities = this.dicomTagsFilters?.ModalitiesInStudy;
        if (currentModalities && currentModalities !== "NONE") {
          const selected = currentModalities.split("\\");
          for (const mod of this.modalitiesList) {
            this.filters.ModalitiesInStudy[mod] = selected.includes(mod);
          }
        } else if (currentModalities === "NONE") {
          // Ничего не выбрано
          for (const mod of this.modalitiesList) {
            this.filters.ModalitiesInStudy[mod] = false;
          }
        } else {
          // Все выбраны (нет фильтра)
          for (const mod of this.modalitiesList) {
            this.filters.ModalitiesInStudy[mod] = true;
          }
        }

        // Теги
        this.filters.labels = Array.isArray(this.labelFilters) ? [...this.labelFilters] : [];
      } finally {
        this.isLoadingFilters = false;
      }
    },
    close() {
      this.$emit("close");
      // Сброс drag состояния
      this.isDragging = false;
      this.modalContentStyle = {};
    },
    clearFilters() {
      this.filters.PatientName = "";
      this.filters.PatientID = "";
      this.filters.AccessionNumber = "";
      this.filters.StudyDescription = "";
      this.studyDateRange = null;
      this.birthDateRange = null;
      this.filters.labels = [];

      // Сбросить модальности (все выбраны)
      for (const mod of this.modalitiesList) {
        this.filters.ModalitiesInStudy[mod] = true;
      }
    },
    async applyFilters() {
      // Защита от повторного применения
      if (this.isApplying) return;
      this.isApplying = true;

      try {
        // 1. Очищаем текущие фильтры в store
        await this.$store.dispatch("studies/clearFilterNoReload");

        const appliedFilters = {};

        // 2. Текстовые поля
        if (this.filters.PatientName?.trim()) {
          await this.$store.dispatch("studies/updateFilterNoReload", {
            dicomTagName: "PatientName",
            value: this.filters.PatientName.trim(),
          });
          appliedFilters.PatientName = this.filters.PatientName.trim();
        }

        if (this.filters.PatientID?.trim()) {
          await this.$store.dispatch("studies/updateFilterNoReload", {
            dicomTagName: "PatientID",
            value: this.filters.PatientID.trim(),
          });
          appliedFilters.PatientID = this.filters.PatientID.trim();
        }
        if (this.filters.AccessionNumber?.trim()) {
          await this.$store.dispatch("studies/updateFilterNoReload", {
            dicomTagName: "AccessionNumber",
            value: this.filters.AccessionNumber.trim(),
          });
          appliedFilters.AccessionNumber = this.filters.AccessionNumber.trim();
        }
        if (this.filters.StudyDescription?.trim()) {
          await this.$store.dispatch("studies/updateFilterNoReload", {
            dicomTagName: "StudyDescription",
            value: this.filters.StudyDescription.trim(),
          });
          appliedFilters.StudyDescription = this.filters.StudyDescription.trim();
        }

        // 3. Дата исследования (конвертация из DatePicker в DICOM формат)
        if (this.studyDateRange && (Array.isArray(this.studyDateRange) ? this.studyDateRange[0] : this.studyDateRange)) {
          try {
            const dicomDate = dateHelpers.dicomDateFromDatePicker(this.studyDateRange);
            if (dicomDate) {
              await this.$store.dispatch("studies/updateFilterNoReload", {
                dicomTagName: "StudyDate",
                value: dicomDate,
              });
              appliedFilters.StudyDate = dicomDate;
            }
          } catch (e) {
            console.error("Failed to convert StudyDate:", e);
          }
        }

        // 4. Дата рождения
        if (this.birthDateRange && (Array.isArray(this.birthDateRange) ? this.birthDateRange[0] : this.birthDateRange)) {
          try {
            const dicomDate = dateHelpers.dicomDateFromDatePicker(this.birthDateRange);
            if (dicomDate) {
              await this.$store.dispatch("studies/updateFilterNoReload", {
                dicomTagName: "PatientBirthDate",
                value: dicomDate,
              });
              appliedFilters.PatientBirthDate = dicomDate;
            }
          } catch (e) {
            console.error("Failed to convert PatientBirthDate:", e);
          }
        }

        // 5. Модальности (обработка трех состояний: все/ничего/выборочно)
        const selectedModalities = Object.entries(this.filters.ModalitiesInStudy)
          .filter(([_, isSelected]) => isSelected)
          .map(([modality]) => modality);

        const allModalitiesSelected = selectedModalities.length === this.modalitiesList.length;
        const noModalitiesSelected = selectedModalities.length === 0;

        if (!allModalitiesSelected && !noModalitiesSelected) {
          // Выборочные модальности
          await this.$store.dispatch("studies/updateFilterNoReload", {
            dicomTagName: "ModalitiesInStudy",
            value: selectedModalities.join("\\"),
          });
          appliedFilters.ModalitiesInStudy = selectedModalities.join("\\");
        } else if (noModalitiesSelected) {
          // Ничего не выбрано - специальное значение NONE для пустого результата
          await this.$store.dispatch("studies/updateFilterNoReload", {
            dicomTagName: "ModalitiesInStudy",
            value: "NONE",
          });
          appliedFilters.ModalitiesInStudy = "NONE";
        }
        // Если все выбраны - не добавляем фильтр (пустая строка = все)

        // 6. Метки (labels)
        if (this.filters.labels && this.filters.labels.length > 0) {
          await this.$store.dispatch("studies/updateLabelFilterNoReload", {
            labels: [...this.filters.labels],
            constraint: "Any", // или "All" если нужно
          });
          appliedFilters.labels = [...this.filters.labels];
        }

        // 7. Перезагружаем список исследований
        await this.$store.dispatch("studies/reloadFilteredStudies");

        // 8. Эмитим событие с примененными фильтрами для родителя (App.vue или StudyList)
        this.$emit("apply-filters", appliedFilters);

        // 9. Закрываем модалку
        this.close();

      } catch (error) {
        console.error("Error applying mobile filters:", error);
        // Можно добавить показ toast с ошибкой
        this.$emit("apply-error", error.message);
      } finally {
        this.isApplying = false;
      }
    },
    toggleModality(mod) {
      const allSelected = this.selectedModalitiesCount === this.modalitiesList.length;

      if (allSelected) {
        // 🆕 Режим "только эта модальность": сбрасываем все, выбираем только кликнутую
        for (const m of this.modalitiesList) {
          this.filters.ModalitiesInStudy[m] = false;
        }
        this.filters.ModalitiesInStudy[mod] = true;
      } else {
        // 🆕 Обычный toggle режим
        this.filters.ModalitiesInStudy[mod] = !this.filters.ModalitiesInStudy[mod];
      }
    },
    selectAllModalities() {
      for (const mod of this.modalitiesList) {
        this.filters.ModalitiesInStudy[mod] = true;
      }
    },

    deselectAllModalities() {
      for (const mod of this.modalitiesList) {
        this.filters.ModalitiesInStudy[mod] = false;
      }
    },
    toggleLabel(label) {
      const idx = this.filters.labels.indexOf(label);
      if (idx === -1) {
        this.filters.labels.push(label);
      } else {
        this.filters.labels.splice(idx, 1);
      }
    },
    isLabelSelected(label) {
      return this.filters.labels.includes(label);
    },
    // 🟡 Drag с поддержкой mouse и touch
    onDragStart(e) {
      // Проверяем, что drag начат с хедера
      if (!e.target.closest(".modal-header")) return;

      this.isDragging = true;
      this.startY = e.type.includes("touch") ? e.touches[0].clientY : e.clientY;

      // Добавляем глобальные обработчики для отслеживания за пределами модалки
      if (e.type.includes("touch")) {
        document.addEventListener("touchmove", this.onDragMove, { passive: false });
        document.addEventListener("touchend", this.onDragEnd);
      } else {
        document.addEventListener("mousemove", this.onDragMove);
        document.addEventListener("mouseup", this.onDragEnd);
      }
    },
    onDragMove(e) {
      if (!this.isDragging) return;
      e.preventDefault(); // Предотвращаем скролл при drag

      this.currentY = e.type.includes("touch") ? e.touches[0].clientY : e.clientY;
      const diff = this.currentY - this.startY;

      if (diff > 0) {
        const newHeight = this.modalHeight - (diff / window.innerHeight) * 100;
        if (newHeight > 20) {
          // 🟡 Используем reactive style вместо прямой манипуляции DOM
          this.modalContentStyle = { height: `${newHeight}vh` };
        }
      }
    },
    onDragEnd() {
      if (!this.isDragging) return;
      this.isDragging = false;

      // Удаляем глобальные обработчики
      document.removeEventListener("touchmove", this.onDragMove);
      document.removeEventListener("touchend", this.onDragEnd);
      document.removeEventListener("mousemove", this.onDragMove);
      document.removeEventListener("mouseup", this.onDragEnd);

      const diff = this.currentY - this.startY;
      if (diff > 100) {
        this.close();
      } else {
        // Возвращаем к исходной высоте с анимацией
        this.modalContentStyle = {
          height: `${this.modalHeight}vh`,
          transition: "height 0.2s ease"
        };
        // Убираем transition после завершения
        setTimeout(() => {
          this.modalContentStyle = {};
        }, 200);
      }
    },
  },
};
</script>

<template>
  <Teleport to="body">
    <Transition name="modal">
      <div v-if="visible" class="mobile-filters-overlay" @click.self="close" @touchstart="onDragStart"
        @mousedown="onDragStart" :class="{ 'is-dragging': isDragging }">
        <div class="mobile-filters-modal" :style="computedModalStyle" :class="{ 'no-transition': isDragging }">
          <!-- Header -->
          <div class="modal-header">
            <div class="drag-handle"></div>
            <div class="header-content">
              <div class="header-title">
                <i class="bi bi-funnel-fill"></i>
                <span>{{ $t('mobile_filters.title') }}</span>
              </div>
              <button class="close-btn" @click="close">
                <i class="bi bi-x-lg"></i>
              </button>
            </div>
          </div>

          <!-- Body -->
          <div class="modal-body">
            <!-- ФИО пациента -->
            <div class="filter-group">
              <label class="filter-label">
                <i class="bi bi-person"></i>
                {{ $t('dicom_tags.PatientName') }}
              </label>
              <input type="text" v-model="filters.PatientName" class="filter-input"
                :placeholder="$t('mobile_filters.patient_name_placeholder')" />
            </div>

            <!-- ID пациента -->
            <div class="filter-group">
              <label class="filter-label">
                <i class="bi bi-card-text"></i>
                {{ $t('dicom_tags.PatientID') }}
              </label>
              <input type="text" v-model="filters.PatientID" class="filter-input"
                :placeholder="$t('mobile_filters.patient_id_placeholder') || 'ID12345'" />
            </div>
            <div class="filter-group">
              <label class="filter-label">
                <i class="bi bi-upc-scan"></i> <!-- или bi-hash -->
                {{ $t('dicom_tags.AccessionNumber') }}
              </label>
              <input type="text" v-model="filters.AccessionNumber" class="filter-input"
                :placeholder="$t('mobile_filters.accession_placeholder') || 'AE1234'" />
            </div>
            <!-- Дата исследования -->
            <div class="filter-group">
              <label class="filter-label">
                <i class="bi bi-calendar-event"></i>
                {{ $t('dicom_tags.StudyDate') }}
              </label>
              <Datepicker v-model="studyDateRange" :enable-time-picker="false" range :format="datePickerFormat"
                :preview-format="datePickerFormat" text-input hide-input-icon :dark="isDarkMode"
                :placeholder="$t('mobile_filters.select_date') || 'Выберите дату'" class="mobile-datepicker" />
            </div>

            <!-- Дата рождения -->
            <div class="filter-group">
              <label class="filter-label">
                <i class="bi bi-cake2"></i>
                {{ $t('dicom_tags.PatientBirthDate') }}
              </label>
              <Datepicker v-model="birthDateRange" :enable-time-picker="false" range :format="datePickerFormat"
                :preview-format="datePickerFormat" text-input hide-input-icon :dark="isDarkMode"
                :placeholder="$t('mobile_filters.select_date') || 'Выберите дату'" class="mobile-datepicker" />
            </div>

            <!-- Описание исследования -->
            <div class="filter-group">
              <label class="filter-label">
                <i class="bi bi-file-text"></i>
                {{ $t('dicom_tags.StudyDescription') }}
              </label>
              <input type="text" v-model="filters.StudyDescription" class="filter-input"
                :placeholder="$t('mobile_filters.description_placeholder') || 'CHEST, HEAD...'" />
            </div>

            <!-- Модальности -->

            <div class="filter-group">
              <span v-if="selectedModalitiesCount === 1 && modalitiesList.length > 1" class="single-mode-hint">
                <i class="bi bi-info-circle"></i>
                {{ $t('mobile_filters.single_modality_mode') }}
                <small>{{ $t('mobile_filters.single_modality_mode_hint') }}</small>
              </span>
              <label class="filter-label">
                <i class="bi bi-grid-3x3"></i>
                {{ $t('dicom_tags.ModalitiesInStudy') }}
                <span class="modality-count">({{ selectedModalitiesCount }}/{{ modalitiesList.length }})</span>
              </label>
              <div class="modalities-grid">
                <button v-for="mod in modalitiesList" :key="mod" class="modality-chip"
                  :class="{ selected: filters.ModalitiesInStudy[mod] }" @click="toggleModality(mod)">
                  {{ mod }}
                </button>
              </div>
              <div class="modality-actions">
                <button class="link-btn" @click="selectAllModalities">
                  {{ $t('all_modalities') }}
                </button>
                <span class="separator">|</span>
                <button class="link-btn" @click="deselectAllModalities">
                  {{ $t('no_modalities') }}
                </button>
              </div>
            </div>

            <!-- Теги -->
            <div v-if="allLabels && allLabels.length > 0" class="filter-group">
              <label class="filter-label">
                <i class="bi bi-tags"></i>
                {{ $t('labels.study_details_title') }}
              </label>
              <div class="labels-grid">
                <button v-for="label in allLabels" :key="label" class="label-chip"
                  :class="{ selected: isLabelSelected(label) }" @click="toggleLabel(label)">
                  <i class="bi bi-tag-fill"></i>
                  {{ label }}
                </button>
              </div>
            </div>
          </div>

          <!-- Footer -->
          <div class="modal-footer">
            <button class="btn-reset" @click="clearFilters" :disabled="!hasAnyFilter || isApplying">
              <i class="bi bi-x-circle"></i>
              {{ $t('clear_filter') }}
            </button>
            <button class="btn-apply" @click="applyFilters" :disabled="isApplying">
              <i class="bi bi-check-lg"></i>
              <span v-if="isApplying">...</span>
              <span v-else>{{ $t('mobile_filters.apply') }}</span>
            </button>
          </div>
        </div>
      </div>
    </Transition>
  </Teleport>
</template>

<style scoped>
/* =============================================================================
   MOBILE FILTERS MODAL - Bottom Sheet Style
   ============================================================================= */

/* Overlay */
.mobile-filters-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.6);
  backdrop-filter: blur(4px);
  z-index: 10000;
  display: flex;
  align-items: flex-end;
  justify-content: center;
  touch-action: none;
  /* 🟡 Предотвращаем конфликты touch/scroll */
}

.mobile-filters-overlay.is-dragging {
  cursor: grabbing;
}

/* Modal Container */
.mobile-filters-modal {
  width: 100%;
  max-width: 100%;
  height: 75vh;
  /* Базовая высота, переопределяется через :style */
  max-height: 85vh;
  background: linear-gradient(180deg, #1e293b 0%, #0f172a 100%);
  border-radius: 20px 20px 0 0;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  box-shadow: 0 -10px 40px rgba(0, 0, 0, 0.5);
  transition: height 0.3s ease;
  /* Плавная анимация высоты */
}

.mobile-filters-modal.no-transition {
  transition: none;
  /* Отключаем анимацию при drag */
}

/* Header */
.modal-header {
  padding: 12px 16px 8px;
  border-bottom: 1px solid rgba(255, 255, 255, 0.1);
  flex-shrink: 0;
  cursor: grab;
  user-select: none;
}

.modal-header:active {
  cursor: grabbing;
}

.drag-handle {
  width: 40px;
  height: 4px;
  background: #64748b;
  border-radius: 2px;
  margin: 0 auto 12px;
}

.header-content {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.header-title {
  display: flex;
  align-items: center;
  gap: 10px;
  font-size: 18px;
  font-weight: 600;
  color: #e2e8f0;
  padding-right: 10px;
}

.header-title i {
  color: #3b82f6;
  font-size: 20px;
}

.close-btn {
  width: 36px;
  height: 36px;
  border: none;
  background: rgba(255, 255, 255, 0.1);
  border-radius: 50%;
  color: #94a3b8;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  transition: all 0.2s ease;
}

.close-btn:hover {
  background: rgba(239, 68, 68, 0.2);
  color: #ef4444;
}

/* Body */
.modal-body {
  flex: 1;
  overflow-y: auto;
  padding: 16px;
  display: flex;
  flex-direction: column;
  gap: 16px;
  touch-action: pan-y;
  /* Разрешаем вертикальный скролл внутри */
}

/* Filter Group */
.filter-group {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.filter-label {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 13px;
  font-weight: 500;
  color: #94a3b8;
}

.filter-label i {
  color: #3b82f6;
  font-size: 14px;
}

.modality-count {
  font-size: 11px;
  color: #64748b;
  margin-left: auto;
}

/* Filter Input */
.filter-input {
  width: 100%;
  padding: 12px 14px;
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 10px;
  color: #e2e8f0;
  font-size: 14px;
  transition: all 0.2s ease;
}

.filter-input:focus {
  outline: none;
  border-color: #3b82f6;
  background: rgba(59, 130, 246, 0.1);
  box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);
}

.filter-input::placeholder {
  color: #64748b;
}

/* Datepicker Override */
.mobile-datepicker :deep(.dp__input) {
  background: rgba(255, 255, 255, 0.05) !important;
  border: 1px solid rgba(255, 255, 255, 0.1) !important;
  border-radius: 10px !important;
  color: #e2e8f0 !important;
  padding: 12px 14px !important;
}

.mobile-datepicker :deep(.dp__input:focus) {
  border-color: #3b82f6 !important;
}

/* Modalities Grid */
.modalities-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.modality-chip {
  padding: 8px 14px;
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 20px;
  color: #94a3b8;
  font-size: 13px;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.2s ease;
}

.modality-chip:hover {
  background: rgba(59, 130, 246, 0.1);
  border-color: rgba(59, 130, 246, 0.3);
}

.modality-chip.selected {
  background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
  border-color: #3b82f6;
  color: white;
  box-shadow: 0 2px 8px rgba(59, 130, 246, 0.3);
}

.modality-actions {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-top: 4px;
}

.link-btn {
  background: none;
  border: none;
  color: #3b82f6;
  font-size: 12px;
  cursor: pointer;
  padding: 0;
}

.link-btn:hover {
  text-decoration: underline;
}

.separator {
  color: #64748b;
  font-size: 12px;
}

/* Labels Grid */
.labels-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.label-chip {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px 12px;
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 16px;
  color: #94a3b8;
  font-size: 12px;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.2s ease;
}

.label-chip i {
  font-size: 10px;
}

.label-chip:hover {
  background: rgba(139, 92, 246, 0.1);
  border-color: rgba(139, 92, 246, 0.3);
}

.label-chip.selected {
  background: linear-gradient(135deg, #8b5cf6 0%, #7c3aed 100%);
  border-color: #8b5cf6;
  color: white;
  box-shadow: 0 2px 8px rgba(139, 92, 246, 0.3);
}

/* Footer */
.modal-footer {
  display: flex;
  gap: 12px;
  padding: 16px;
  border-top: 1px solid rgba(255, 255, 255, 0.1);
  background: rgba(0, 0, 0, 0.2);
  flex-shrink: 0;
  padding-bottom: calc(16px + env(safe-area-inset-bottom, 0));
}

.btn-reset,
.btn-apply {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding: 14px 20px;
  border: none;
  border-radius: 12px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s ease;
}

.btn-reset {
  background: rgba(255, 255, 255, 0.1);
  color: #94a3b8;
}

.btn-reset:hover:not(:disabled) {
  background: rgba(239, 68, 68, 0.1);
  color: #ef4444;
}

.btn-reset:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.btn-apply {
  background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
  color: white;
  box-shadow: 0 4px 12px rgba(59, 130, 246, 0.3);
}

.btn-apply:hover:not(:disabled) {
  background: linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%);
  transform: translateY(-1px);
  box-shadow: 0 6px 16px rgba(59, 130, 246, 0.4);
}

.btn-apply:disabled {
  opacity: 0.6;
  cursor: not-allowed;
  transform: none;
}

.btn-apply:active:not(:disabled) {
  transform: translateY(0);
}

/* Transitions */
.modal-enter-active,
.modal-leave-active {
  transition: all 0.3s ease;
}

.modal-enter-active .mobile-filters-modal,
.modal-leave-active .mobile-filters-modal {
  transition: transform 0.3s ease, height 0.3s ease;
}

.modal-enter-from,
.modal-leave-to {
  opacity: 0;
}

.modal-enter-from .mobile-filters-modal,
.modal-leave-to .mobile-filters-modal {
  transform: translateY(100%);
}

/* Scrollbar */
.modal-body::-webkit-scrollbar {
  width: 4px;
}

.modal-body::-webkit-scrollbar-track {
  background: transparent;
}

.modal-body::-webkit-scrollbar-thumb {
  background: rgba(255, 255, 255, 0.2);
  border-radius: 2px;
}

.single-mode-hint {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 11px;
  color: #3b82f6;
  margin-left: 8px;
  font-weight: 500;
}

.single-mode-hint small {
  font-weight: 400;
  color: #64748b;
  margin-left: 4px;
}

.single-mode-hint i {
  font-size: 12px;
}
</style>