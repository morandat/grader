require 'csv'

raise "need csv file" if ARGV.empty?

def round4 score
  (score * 4).ceil / 4.0
end

def grade row, scale
  login = row[0]
  mark = 0
  for i in 0...(row.length/2)
if row[2*i + 1].to_f == 1
      mark += scale[2*i+1]
    else
      passed = [0, row[2 * (i + 1)].to_f - 1].max
      max = scale[2*(i+1)] - 1
      mark += round4(passed.to_f / max.to_f)*scale[2*i + 1]
    end
  end
  mark = 20.0 if mark > 20
  puts "#{login} ; #{mark}"
  mark
end

def median array
  sorted = array.sort
  len = sorted.length
  return (sorted[(len - 1) / 2] + sorted[len / 2]) / 2.0
end

def mean array
  array.inject{ |sum, el| sum + el }.to_f / array.size
end

scale = []
marks = []
CSV.foreach(ARGV[0], col_sep: ';') do |row|
  if scale.empty?
    scale = row.map {|i| i.to_f}
  else
    marks << grade(row, scale)
  end
end

puts
puts "moyenne ; #{marks.inject{ |sum, el| sum + el }.to_f / marks.size}"
puts "medianne ; #{median marks}"
puts "max ; #{marks.max}"
puts "min ; #{marks.min}"
