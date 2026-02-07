<script>
/**
 * StudyListFilters.vue
 *
 * Отвечает за ВСЮ фильтрующую UI:
 *
 * 1) <thead> таблицы — заголовки колонок, инпуты фильтров, datepicker-ы,
 *    дропдаун модальностей, фильтр меток (labels)
 *
 * 2) Quick-filters и сортировка для card-view — через Teleport
 *    в контейнер #study-quick-filters-slot, который родитель размещает
 *    в actions-row.
 *
 * 3) Кнопка поиска — через Teleport в #study-search-slot.
 *
 * Состояние фильтров хранится в родителе (StudyList.vue).
 * Этот компонент получает значения через props и отправляет
 * изменения через emit.
 */
import LabelsEditor from "./LabelsEditor.vue";
import { mapState } from "vuex";
import { translateDicomTag } from "../locales/i18n";
import SourceType from "../helpers/source-type";
import $ from "jquery";

export default {
  name: "StudyListFilters",
  components: { LabelsEditor },
  props: {
    filterStudyDate: { type: String, default: "" },
    // --- Вид ---
    isTableView: { type: Boolean, default: true },
    isCardView: { type: Boolean, default: false },

    // --- Конфигурация колонок ---
    columns: { type: Object, required: true },
    widthColum1: { type: String, default: "3%" },
    colSpanClearFilter: { type: Number, default: 1 },
    hasPrimaryViewerIcon: { type: Boolean, default: false },
    hasQuickActionsColumn: { type: Boolean, default: false },
    hasPdfReportIcon: { type: Boolean, default: false },

    // --- Labels filter layout ---
    isMultiLabelsFilterVisible: { type: Boolean, default: false },
    colSpanBeforeMultiLabelsFilter: { type: Number, default: 1 },
    colSpanMultiLabelsFilter: { type: Number, default: 4 },
    colSpanAfterMultiLabelsFilter: { type: Number, default: 3 },

    // --- Source ---
    sourceType: { type: Number, default: SourceType.LOCAL_ORTHANC },

    // --- Ordering ---
    currentOrderByTag: { type: [String, null], default: null },
    currentOrderDirection: { type: String, default: "ASC" },

    // --- Filter values (from parent) ---
    filterStudyDateForDatePicker: { default: null },
    filterPatientBirthDateForDatePicker: { default: null },
    filterGenericTags: { type: Object, default: () => ({}) },
    filterModalities: { type: Object, default: () => ({}) },
    allModalities: { type: Boolean, default: true },
    noneModalities: { type: Boolean, default: false },
    datePickerPresetRanges: { type: Array, default: () => [] },
    isDarkMode: { type: Boolean, default: false },

    // --- Labels ---
    filterLabels: { type: Array, default: () => [] },
    multiLabelsFilterLabelsConstraint: { type: String, default: "All" },
    multiLabelsComponentKey: { type: Number, default: 0 },

    // --- Card view controls ---
    cardViewSort: { type: String, default: "date-desc" },
    cardViewSortOptions: { type: Array, default: () => [] },
    activeModalityFilter: { type: [String, null], default: null },

    // --- Search ---
    isSearchButtonEnabled: { type: Boolean, default: false },
    isSearching: { type: Boolean, default: false },
  },
  emits: [
    // Filter value changes
    "update:filterStudyDateForDatePicker",
    "update:filterPatientBirthDateForDatePicker",
    "update:filterGenericTags",
    "update:filterModalities",
    "update:allModalities",
    "update:noneModalities",
    "update:multiLabelsFilterLabelsConstraint",
    "update:cardViewQuickFilter",
    "update:cardViewSort",
    // Actions
    "search",
    "clear-filters",
    "toggle-order",
    "toggle-modality-filter",
    "close-modality-filter",
    "modality-filter-clicked",
    "labels-updated",
    "clear-active-modality-filter",
    "set-card-quick-filter",
  ],
  data() {
    return {
      /** Флаг для отложенного Teleport (ждём mount целевых контейнеров) */
      isTeleportReady: false,
    };
  },
  computed: {
    ...mapState({
      uiOptions: (state) => state.configuration.uiOptions,
      hasExtendedFind: (state) => state.configuration.hasExtendedFind,
      canShowStudiesWithoutLabels: (state) => state.labels.canShowStudiesWithoutLabels,
    }),
    datePickerFormat() {
      return this.uiOptions.DateFormat;
    },
    quickFilterOptions() {
      return [
        { value: "all", label: this.$t("all_modalities") || "Все" },
        { value: "today", label: this.$t("today") || "Сегодня" },
        { value: "week", label: this.$t("date_picker.last_week") || "Неделя" },
        { value: "month", label: this.$t("date_picker.last_month") || "Месяц" },
        { value: "year", label: this.$t("date_picker.last_12_months") || "Год" },
      ];
    },
    // --- Proxy для v-model datepicker-ов ---
    localStudyDateForDatePicker: {
      get() { return this.filterStudyDateForDatePicker; },
      set(v) { this.$emit("update:filterStudyDateForDatePicker", v); },
    },
    localPatientBirthDateForDatePicker: {
      get() { return this.filterPatientBirthDateForDatePicker; },
      set(v) { this.$emit("update:filterPatientBirthDateForDatePicker", v); },
    },
    localMultiLabelsConstraint: {
      get() { return this.multiLabelsFilterLabelsConstraint; },
      set(v) { this.$emit("update:multiLabelsFilterLabelsConstraint", v); },
    },
    localCardViewSort: {
      get() { return this.cardViewSort; },
      set(v) { this.$emit("update:cardViewSort", v); },
    },
  },
  mounted() {
    // Даём один тик чтобы родительский DOM (actions row) успел отрисоваться
    this.$nextTick(() => {
      this.isTeleportReady = true;
    });
  },
  methods: {
    isQuickFilterActive(filterType) {
      // ← ДОБАВИТЬ ПРОВЕРКУ
      if (!this.filterStudyDate) return filterType === "all";

      const today = new Date();
      today.setHours(0, 0, 0, 0);
      const fmt = (d) => {
        const y = d.getFullYear();
        const m = String(d.getMonth() + 1).padStart(2, "0");
        const day = String(d.getDate()).padStart(2, "0");
        return `${y}${m}${day}`;
      };

      let expectedDate = "";
      switch (filterType) {
        case "today": expectedDate = fmt(today); break;
        case "week": { const d = new Date(today); d.setDate(today.getDate() - 7); expectedDate = `${fmt(d)}-${fmt(today)}`; break; }
        case "month": { const d = new Date(today); d.setMonth(today.getMonth() - 1); expectedDate = `${fmt(d)}-${fmt(today)}`; break; }
        case "year": { const d = new Date(today); d.setFullYear(today.getFullYear() - 1); expectedDate = `${fmt(d)}-${fmt(today)}`; break; }
        default: return false;
      }

      return this.filterStudyDate === expectedDate;
    },
    // --- Column helpers ---
    columnTitle(tagName) {
      if (tagName === "seriesCount") return this.$i18n.t("series_count_header");
      if (tagName === "instancesCount") return this.$i18n.t("instances_count_header");
      if (tagName === "seriesAndInstancesCount") return this.$i18n.t("series_and_instances_count_header");
      if (tagName === "modalities") return translateDicomTag(this.$i18n.t, this.$i18n.te, "ModalitiesInStudy");
      return translateDicomTag(this.$i18n.t, this.$i18n.te, tagName);
    },
    columnTooltip(tagName) {
      return this.columnTitle(tagName);
    },
    columnWidth(tagName) {
      if (tagName in this.columns) return this.columns[tagName].width;
      return this.columns["undefined"].width;
    },
    isOrderable(tagName) {
      if (this.sourceType !== SourceType.LOCAL_ORTHANC || !this.hasExtendedFind) return false;
      if (tagName in this.columns) return !!this.columns[tagName].isOrderable;
      return false;
    },
    isOrderTagUp(tagName) {
      return tagName === this.currentOrderByTag && this.currentOrderDirection === "DESC";
    },
    isOrderTagDown(tagName) {
      return tagName === this.currentOrderByTag && this.currentOrderDirection === "ASC";
    },
    hasFilter(tagName) {
      return ["seriesCount", "instancesCount", "seriesAndInstancesCount"].indexOf(tagName) === -1;
    },
    getFilterPlaceholder(tagName) {
      if (tagName in this.columns && this.columns[tagName].placeholder) return this.columns[tagName].placeholder;
      return "search-text";
    },
    getFilterClass(tagName) {
      const value = this.filterGenericTags[tagName];
      if (value != null && value.length > 0 && !this.isFilterLongEnough(tagName, value)) {
        return "is-invalid-filter";
      }
      return "";
    },
    getMinimalFilterLength(tagName) {
      if (["AccessionNumber", "PatientName", "PatientID", "StudyDescription"].includes(tagName)) {
        if (this.uiOptions.StudyListSearchMode === "search-as-you-type") {
          return this.uiOptions.StudyListSearchAsYouTypeMinChars;
        }
      } else if (["PatientBirthDate", "StudyDate"].includes(tagName)) {
        return 8;
      }
      return 0;
    },
    isFilterLongEnough(tagName, value) {
      return value.length >= this.getMinimalFilterLength(tagName);
    },

    // --- Event proxies ---
    onToggleOrder(ev, tagName) {
      ev.preventDefault();
      ev.stopPropagation();
      this.$emit("toggle-order", tagName);
    },
    onToggleModalityFilter(ev) {
      this.$emit("toggle-modality-filter", ev);
    },
    onModalityFilterClicked(ev) {
      ev.stopPropagation();
    },
    onCloseModalityFilter(ev) {
      $("#dropdown-modalities-button").click();
      ev.preventDefault();
      ev.stopPropagation();
    },
    onGenericTagInput(tagName, value) {
      // Обновляем через emit - родитель обновит filterGenericTags
      const updated = { ...this.filterGenericTags, [tagName]: value };
      this.$emit("update:filterGenericTags", updated);
    },
    onModalityCheckboxChange(modality, value) {
      const updated = { ...this.filterModalities, [modality]: value };
      this.$emit("update:filterModalities", updated);
    },
  },
};
</script>

<template>
  <!-- ================================================================
       СЕКЦИЯ 1: Table Header (<thead>) — колонки + фильтры + labels
       ================================================================ -->
  <thead class="sticky-top">
    <!-- Row 1: Заголовки колонок -->
    <tr class="study-column-titles">
      <th :width="widthColum1" max-width="40px" scope="col"></th>
      <th v-if="hasPrimaryViewerIcon" scope="col" class="download-buttons-group"></th>
      <th v-if="hasQuickActionsColumn" width="4%" max-width="70px" scope="col"></th>
      <th v-if="hasPdfReportIcon" width="3%" max-width="30px" scope="col"></th>
      <th v-for="columnTag in uiOptions.StudyListColumns" :key="columnTag" data-bs-toggle="tooltip"
        :title="columnTooltip(columnTag)" :width="columnWidth(columnTag)" class="study-table-title">
        <div class="title-container">
          <div v-if="isOrderable(columnTag)" class="title-text is-orderable" @click="onToggleOrder($event, columnTag)">
            {{ columnTitle(columnTag) }}
          </div>
          <div v-else class="title-text">
            {{ columnTitle(columnTag) }}
          </div>
          <div v-if="isOrderTagUp(columnTag)" class="title-arrow">
            <i class="bi bi-arrow-up"></i>
          </div>
          <div v-if="isOrderTagDown(columnTag)" class="title-arrow">
            <i class="bi bi-arrow-down"></i>
          </div>
        </div>
      </th>
    </tr>

    <!-- Row 2: Инпуты фильтров -->
    <tr class="study-table-filters" @keyup.enter="$emit('search')">
      <th scope="col" :colspan="colSpanClearFilter">
        <div class="d-flex justify-content-end">
          <button @click="$emit('clear-filters')" type="button"
            class="form-control study-list-filter btn filter-button btn-sm" data-bs-toggle="tooltip"
            :title="$t('clear_filter')">
            <i class="bi bi-x-lg"></i>
          </button>
        </div>
      </th>
      <th v-for="columnTag in uiOptions.StudyListColumns" :key="columnTag">
        <!-- StudyDate datepicker -->
        <div v-if="columnTag === 'StudyDate'">
          <Datepicker v-model="localStudyDateForDatePicker" :enable-time-picker="false" range
            :preset-dates="datePickerPresetRanges" :format="datePickerFormat" :preview-format="datePickerFormat"
            text-input arrow-navigation hide-input-icon :highlight="{ weekdays: [6, 0] }" :dark="isDarkMode">
            <template #yearly="{ label, range, presetDate }">
              <span @click="presetDate(range)">{{ label }}</span>
            </template>
          </Datepicker>
        </div>

        <!-- Modalities dropdown -->
        <div v-else-if="columnTag === 'modalities'" class="dropdown">
          <button type="button" class="btn btn-default btn-sm filter-button dropdown-toggle" data-bs-toggle="dropdown"
            id="dropdown-modalities-button" aria-expanded="false">
            <span class="fa fa-list"></span>&nbsp;<span class="caret"></span>
          </button>
          <ul class="dropdown-menu" aria-labelledby="dropdown-modalities-button" @click="onModalityFilterClicked"
            id="modality-filter-dropdown">
            <li>
              <label class="dropdown-item">
                <input type="checkbox" data-value="all" @click="onToggleModalityFilter"
                  :checked="allModalities" />&nbsp;{{
                    $t("all_modalities") }}
              </label>
            </li>
            <li>
              <label class="dropdown-item">
                <input type="checkbox" data-value="none" @click="onToggleModalityFilter"
                  :checked="noneModalities" />&nbsp;{{
                    $t("no_modalities") }}
              </label>
            </li>
            <li>
              <hr class="dropdown-divider" />
            </li>
            <li v-for="modality in uiOptions.ModalitiesFilter" :key="modality">
              <label class="dropdown-item">
                <input type="checkbox" :data-value="modality" :checked="filterModalities[modality]"
                  @change="onModalityCheckboxChange(modality, $event.target.checked)" />&nbsp;{{ modality }}
              </label>
            </li>
            <li>
              <button class="btn btn-primary mx-5" @click="onCloseModalityFilter">
                {{ $t("close") }}
              </button>
            </li>
          </ul>
        </div>

        <!-- PatientBirthDate datepicker -->
        <div v-else-if="columnTag === 'PatientBirthDate'">
          <Datepicker v-model="localPatientBirthDateForDatePicker" :enable-time-picker="false" range
            :format="datePickerFormat" hide-input-icon :preview-format="datePickerFormat" text-input arrow-navigation
            :highlight="{ weekdays: [6, 0] }" :dark="isDarkMode" />
        </div>

        <!-- Generic text filter -->
        <input v-else-if="hasFilter(columnTag)" type="text" class="form-control study-list-filter"
          :value="filterGenericTags[columnTag]" @input="onGenericTagInput(columnTag, $event.target.value)"
          :placeholder="getFilterPlaceholder(columnTag)" :class="getFilterClass(columnTag)" />
      </th>
    </tr>

    <!-- Row 3: Multi-labels filter -->
    <tr v-if="isMultiLabelsFilterVisible" class="study-table-actions">
      <th :colspan="colSpanBeforeMultiLabelsFilter" scope="col">
        <div class="w-100 d-flex justify-content-end">
          <label class="form-check-label text-end" for="multiLabelsFilter">
            {{ $t("labels.study_details_title") }}
          </label>
        </div>
      </th>
      <th :colspan="colSpanMultiLabelsFilter" scope="col">
        <LabelsEditor id="multiLabelsFilter" :labels="filterLabels" :key="multiLabelsComponentKey" :studyId="null"
          @labelsUpdated="$emit('labels-updated', $event)" :showTitle="false" :isFilter="true" />
      </th>
      <th :colspan="colSpanAfterMultiLabelsFilter" scope="col">
        <div class="w-100 d-flex">
          <input class="form-check-input ms-2 me-1" type="radio" name="multiLabelsFilterAll" id="multiLabelsFilterAll"
            value="All" v-model="localMultiLabelsConstraint" />
          <label class="form-check-label" for="multiLabelsFilterAll">
            {{ $t("labels.filter_labels_constraint_all") }}
          </label>
          <input class="form-check-input ms-2 me-1" type="radio" name="multiLabelsFilterAny" id="multiLabelsFilterAny"
            value="Any" v-model="localMultiLabelsConstraint" />
          <label class="form-check-label" for="multiLabelsFilterAny">
            {{ $t("labels.filter_labels_constraint_any") }}
          </label>
          <input v-if="canShowStudiesWithoutLabels" class="form-check-input ms-2 me-1" type="radio"
            name="multiLabelsFilterWithoutLabels" id="multiLabelsFilterWithoutLabels" value="None"
            v-model="localMultiLabelsConstraint" />
          <label v-if="canShowStudiesWithoutLabels" class="form-check-label" for="multiLabelsFilterWithoutLabels">
            {{ $t("labels.studies_without_labels") }}
          </label>
        </div>
      </th>
    </tr>
  </thead>

  <!-- ================================================================
       СЕКЦИЯ 2: Quick Filters + Sort + Modality badge + Search button
       Teleport в целевые контейнеры внутри actions-row родителя.
       ================================================================ -->

  <!-- Quick filters, sort, modality badge → left group в actions row -->
  <Teleport to="#study-quick-filters-slot" v-if="isTeleportReady">
    <!-- Active modality filter badge -->
    <div v-if="activeModalityFilter" class="active-modality-filter">
      <span class="badge bg-primary">
        {{ activeModalityFilter }}
        <i class="bi bi-x-circle" @click="$emit('clear-active-modality-filter')" title="Clear filter"></i>
      </span>
    </div>

    <!-- Quick date filters (card view only) -->
    <div class="quick-filters">
      <button :title="$t('all')" class="quick-filter-btn" :class="{ active: isQuickFilterActive('all') }"
        @click="$emit('set-card-quick-filter', 'all')">
        <i class="bi bi-list-ul"></i>
      </button>
      <button :title="$t('date_picker.today')" class="quick-filter-btn"
        :class="{ active: isQuickFilterActive('today') }" @click="$emit('set-card-quick-filter', 'today')">
        <i class="bi bi-calendar-day"></i>
      </button>
      <button :title="$t('date_picker.last_week')" class="quick-filter-btn"
        :class="{ active: isQuickFilterActive('week') }" @click="$emit('set-card-quick-filter', 'week')">
        <i class="bi bi-calendar-week"></i>
      </button>
      <button :title="$t('date_picker.last_month')" class="quick-filter-btn"
        :class="{ active: isQuickFilterActive('month') }" @click="$emit('set-card-quick-filter', 'month')">
        <i class="bi bi-calendar-month"></i>
      </button>
      <button :title="$t('date_picker.last_12_months') || $t('last_year')" class="quick-filter-btn"
        :class="{ active: isQuickFilterActive('year') }" @click="$emit('set-card-quick-filter', 'year')">
        <i class="bi bi-calendar"></i>
      </button>
    </div>

    <!-- Sort dropdown (card view only) -->
    <div class="card-sort-dropdown">
      <select id="cardSort" v-model="localCardViewSort" class="form-select form-select-sm">
        <option v-for="option in cardViewSortOptions" :key="option.value" :title="option.title" :value="option.value">
          {{ option.label }}
        </option>
      </select>
    </div>
  </Teleport>

  <!-- Search button → right side в actions row -->
  <Teleport to="#study-search-slot" v-if="isTeleportReady">
    <div v-if="isSearchButtonEnabled" class="search-button-wrapper">
      <button @click="$emit('search')" type="submit" class="btn btn-secondary search-button"
        :class="{ 'is-searching': isSearching, 'is-not-searching': !isSearching }" :title="$t('search')">
        <i v-if="!isSearching" class="fa-solid fa-magnifying-glass"></i>
        <span v-else class="spinner-border spinner-border-sm" role="status" aria-hidden="true"></span>
      </button>
    </div>
  </Teleport>
</template>

<!-- Стили — НЕ scoped, т.к. оригинал был глобальным и другие компоненты на них полагаются -->
<style>
/* Стили для thead и фильтров наследуются из глобального CSS StudyList.
   Здесь только то, что специфично для этого компонента. */
</style>