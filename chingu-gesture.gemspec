require_relative 'lib/chingu_gesture'

Gem::Specification.new do |s|
  s.name        = 'chingu-gesture'
  s.version     = ChinguGesture::VERSION
  s.licenses    = ['MIT']
  s.homepage    = "https://github.com/zombiecalypse/chingu-gesture"
  s.summary     = "Recognize gestures fast"
  s.description = "Recognition for 2d gestures from a stream of points using dynamic time warping"
  s.authors     = ["zombiecalypse"]
  s.email       = 'maergil@gmail.com'
  s.files       = Dir['lib/**/*.rb'] + Dir['ext/**/*.c']
  s.extensions  = ['ext/chingu_gesture/extconf.rb']
end
