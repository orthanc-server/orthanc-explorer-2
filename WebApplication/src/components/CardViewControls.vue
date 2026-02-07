<script>
export default {
    name: 'CardViewControls',

    props: {
        isCardView: Boolean,
        quickFilter: { type: String, default: 'all' },
        sortValue: String,
        sortOptions: { type: Array, required: true },
    },

    emits: ['set-quick-filter', 'update:sort-value'],

    computed: {
        sortModel: {
            get() { return this.sortValue; },
            set(v) { this.$emit('update:sort-value', v); }
        }
    }
}
</script>
<template>
    <div v-if="isCardView" class="d-flex align-items-center gap-2 flex-wrap">
        <!-- Quick filters -->
        <div class="btn-group btn-group-sm" role="group">
            <button type="button" class="btn btn-outline-secondary" :class="{ active: quickFilter === 'all' }"
                @click="$emit('set-quick-filter', 'all')" :title="$t('all')">
                <i class="bi bi-list-ul"></i>
            </button>
            <button type="button" class="btn btn-outline-secondary" :class="{ active: quickFilter === 'today' }"
                @click="$emit('set-quick-filter', 'today')" :title="$t('date_picker.today')">
                <i class="bi bi-calendar-day"></i>
            </button>
            <button type="button" class="btn btn-outline-secondary" :class="{ active: quickFilter === 'week' }"
                @click="$emit('set-quick-filter', 'week')" :title="$t('date_picker.last_week')">
                <i class="bi bi-calendar-week"></i>
            </button>
            <button type="button" class="btn btn-outline-secondary" :class="{ active: quickFilter === 'month' }"
                @click="$emit('set-quick-filter', 'month')" :title="$t('date_picker.last_month')">
                <i class="bi bi-calendar-month"></i>
            </button>
            <button type="button" class="btn btn-outline-secondary" :class="{ active: quickFilter === 'year' }"
                @click="$emit('set-quick-filter', 'year')" :title="$t('date_picker.last_12_months')">
                <i class="bi bi-calendar"></i>
            </button>
        </div>

        <!-- Sort -->
        <select v-model="sortModel" class="form-select form-select-sm" style="width: auto; min-width: 140px;">
            <option v-for="opt in sortOptions" :key="opt.value" :value="opt.value">
                {{ opt.label }}
            </option>
        </select>
    </div>
</template>
