require 'rspec'
require_relative '../lib/chingu_gesture'

describe ChinguGesture do
  subject { ChinguGesture.new }
  before do
    subject.def_gesture(:rl, [[1,0], [0,0]])
    subject.def_gesture(:td) do |p|
      p[0,1]
      p[0,0]
    end
    subject.def_gesture(:lr) do |p|
      p[0,0]
      p[1,0]
    end
    subject.def_gesture(:dt) do |p|
      p[0,0]
      p[0,1]
    end
    subject.def_gesture(:circ) do |p|
      p[0,0]
      p[0,1]
      p[1,1]
      p[1,0]
    end
    20.times do |i|
      subject.def_gesture(:"rl#{i}", [[1,0], [0,0]])
    end
  end

  it "should recognize exact match" do
    subject.add_point(0,0)
    subject.add_point(1,0)
    expect(subject.recognize.first).to(eq(:lr))
  end

  it "should recognize exact match with additional points" do
    subject.add_point(0,0)
    subject.add_point(0,0)
    subject.add_point(1,0)
    subject.add_point(1,0)
    expect(subject.recognize.first).to(eq(:lr))
  end

  it "should recognize match with offset" do
    subject.add_point(100,0)
    subject.add_point(200,0)
    expect(subject.recognize.first).to(eq(:lr))
  end

  it "should recognize match with offset and points in between" do
    subject.add_point(100,0)
    subject.add_point(150,0)
    subject.add_point(175,0)
    subject.add_point(200,0)
    expect(subject.recognize.first).to(eq(:lr))
  end

  it "should recognize match with offset and noisy points in between" do
    subject.add_point(100,0)
    subject.add_point(150,5)
    subject.add_point(175,9)
    subject.add_point(200,15)
    expect(subject.recognize.first).to(eq(:lr))
  end

  it "should recognize complex gestures" do
    (0..10).each do |i|
      subject.add_point((Math.sin(i)*10).to_i, (Math.cos(i)*10).to_i)
    end
    expect(subject.recognize.first).to(eq(:circ))
  end

  it "should gestures after reset" do
    subject.add_point(100,0)
    subject.add_point(150,5)
    subject.add_point(175,9)
    subject.add_point(200,15)
    expect(subject.recognize.first).to(eq(:lr))
    subject.clear
    (0..10).each do |i|
      subject.add_point((Math.sin(i)*10).to_i, (Math.cos(i)*10).to_i)
    end
    expect(subject.recognize.first).to(eq(:circ))
  end

  it "should be blazing fast" do
    require 'benchmark'
    expect(Benchmark.realtime do
      (0..100).each do |i|
        subject.add_point((Math.sin(i)*100).to_i, (Math.cos(i)*100).to_i)
      end
      expect(subject.recognize.first).to(eq(:circ))
    end).to be <= 0.01
  end
end
