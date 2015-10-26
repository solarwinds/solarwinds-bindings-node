var gulp = require('gulp')
var mocha = require('gulp-mocha')
var spawn = require('child_process').spawn
var build = require('./build')

gulp.task('test', function () {
  return gulp.src('test/*.test.js', {
    read: false
  })
  .pipe(tester())
  .once('end', process.exit)
})

gulp.task('rebuild', build)

gulp.task('watch', function () {
  gulp.watch([
    '{lib,test}/**/*.js',
    'build/**/*.node',
    'index.js'
  ], [
    'test'
  ])

  gulp.watch('src/**/*.{cc,h}', [
    'rebuild'
  ])
})

gulp.task('default', [
  'test'
])

//
// Helpers
//

function tester () {
  require('./')
  require('should')

  return mocha({
    reporter: 'spec',
    timeout: 5000
  })
}
