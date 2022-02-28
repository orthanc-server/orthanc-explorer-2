import { createStore, createLogger } from 'vuex'
import configuration from './modules/configuration'
import studies from './modules/studies'

const debug = process.env.NODE_ENV !== 'production'

export default createStore({
  modules: {
    studies,
    configuration
  },
  strict: debug,
  plugins: debug ? [createLogger()] : []
})