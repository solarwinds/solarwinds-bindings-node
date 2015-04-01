var gulp = require('gulp')
var mocha = require('gulp-mocha')
var spawn = require('child_process').spawn

gulp.task('test', function () {
  return gulp.src('test/*.test.js', {
    read: false
  })
  .pipe(tester())
  .once('end', process.exit)
})

gulp.task('rebuild', function (cb) {
  var p = spawn('npm', ['install'])
  p.stdout.pipe(process.stdout)
  p.stderr.pipe(process.stderr)
  p.on('close', cb)
})

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
