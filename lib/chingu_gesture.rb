class ChinguGesture
  VERSION = "1.0"
  # Get all points for debugging, etc
  def to_a
    (0...size).collect do |i|
      [_get_x(i), _get_y(i)]
    end
  end

  def def_gesture(sym, list=nil)
    list ||= yield(ListAdder.new)
    @gesture_translate ||= {}
    @gesture_translate[_add_gesture(list)] = sym
  end

  def recognize
    n, d = _recognize
    [@gesture_translate[n], d]
  end

  def translation
    @gesture_translate.dup
  end

  class ListAdder
    def initialize
      @list = []
    end

    def [](x,y)
      @list << [x,y]
    end
  end
end
require_relative 'chingu_gesture/chingu_gesture'
