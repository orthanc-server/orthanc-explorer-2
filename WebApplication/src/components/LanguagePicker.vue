<script>
import languages from '../store/modules/i18n';
export default {
  name: 'App',
  data() {
    return {
        languages,
      option: ''
    };
  },
  created() {
    this.option = 'English'
  },
  methods: {
    changeLanguage(key, name) {
      this.$i18n.locale = key;
      this.option = name
    }
  },
}
</script>

<template>
  <div class="dropdown">
    <button class="btn btn-primary dropdown-toggle" type="button" id="dropdownMenuButton1" data-bs-toggle="dropdown"
      aria-expanded="false">
      <i class="bi bi-translate"></i> {{option}}
    </button>
    <ul class="dropdown-menu language-picker">
      <li v-for="(lang, i) in languages" :key="`lang-${i}`" :value="lang"><a class="dropdown-item" href="#"
          @click="changeLanguage(`${lang.abbreviation}`,`${lang.name}`)">{{lang.name}}</a></li>
    </ul>
  </div>
</template>

<style>
.language-picker {
    margin-top: var(--details-top-margin);
    font-size: 0.9rem;
    --bs-dropdown-item-padding-y: 0.05rem;
}
</style>
